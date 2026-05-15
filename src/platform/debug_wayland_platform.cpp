#include "platform/debug_wayland_platform.h"

#include "core/log.h"
#include "ui/controls/input.h"

#include <GLES2/gl2.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <linux/input-event-codes.h>
#include <stdexcept>
#include <sys/mman.h>
#include <thread>
#include <unistd.h>

namespace noctalia::platform {
  namespace {
    constexpr wl_registry_listener kRegistryListener{.global = &DebugWaylandPlatform::global,
                                                     .global_remove = &DebugWaylandPlatform::globalRemove};
    constexpr xdg_wm_base_listener kShellListener{.ping = &DebugWaylandPlatform::ping};
    constexpr xdg_surface_listener kXdgListener{.configure = &DebugWaylandPlatform::xdgConfigure};
    constexpr xdg_toplevel_listener kTopListener{.configure = &DebugWaylandPlatform::topConfigure,
                                                 .close = &DebugWaylandPlatform::topClose,
                                                 .configure_bounds = nullptr,
                                                 .wm_capabilities = nullptr};
    constexpr wl_seat_listener kSeatListener{.capabilities = &DebugWaylandPlatform::seatCapabilities,
                                             .name = &DebugWaylandPlatform::seatName};
    constexpr wl_pointer_listener kPointerListener{.enter = &DebugWaylandPlatform::pointerEnter,
                                                   .leave = &DebugWaylandPlatform::pointerLeave,
                                                   .motion = &DebugWaylandPlatform::pointerMotion,
                                                   .button = &DebugWaylandPlatform::pointerButton,
                                                   .axis = &DebugWaylandPlatform::pointerAxis,
                                                   .frame = &DebugWaylandPlatform::pointerFrame,
                                                   .axis_source = &DebugWaylandPlatform::pointerAxisSource,
                                                   .axis_stop = &DebugWaylandPlatform::pointerAxisStop,
                                                   .axis_discrete = &DebugWaylandPlatform::pointerAxisDiscrete,
                                                   .axis_value120 = &DebugWaylandPlatform::pointerAxisValue120,
                                                   .axis_relative_direction =
                                                       &DebugWaylandPlatform::pointerAxisRelativeDirection};
    constexpr wl_keyboard_listener kKeyboardListener{.keymap = &DebugWaylandPlatform::keyboardKeymap,
                                                     .enter = &DebugWaylandPlatform::keyboardEnter,
                                                     .leave = &DebugWaylandPlatform::keyboardLeave,
                                                     .key = &DebugWaylandPlatform::keyboardKey,
                                                     .modifiers = &DebugWaylandPlatform::keyboardModifiers,
                                                     .repeat_info = &DebugWaylandPlatform::keyboardRepeatInfo};
    constexpr EGLint kConfigAttrs[] = {EGL_SURFACE_TYPE,
                                       EGL_WINDOW_BIT,
                                       EGL_RENDERABLE_TYPE,
                                       EGL_OPENGL_ES2_BIT,
                                       EGL_RED_SIZE,
                                       8,
                                       EGL_GREEN_SIZE,
                                       8,
                                       EGL_BLUE_SIZE,
                                       8,
                                       EGL_ALPHA_SIZE,
                                       8,
                                       EGL_NONE};
    constexpr EGLint kContextAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  } // namespace

  DebugWaylandPlatform::~DebugWaylandPlatform() { cleanup(); }

  bool DebugWaylandPlatform::initialize() {
    m_display = wl_display_connect(nullptr);
    if (m_display == nullptr) {
      core::log(core::LogLevel::Error, "wayland", "failed to connect to Wayland display");
      return false;
    }
    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &kRegistryListener, this);
    wl_display_roundtrip(m_display);
    if (m_compositor == nullptr || m_shell == nullptr) {
      core::log(core::LogLevel::Error, "wayland", "missing compositor or xdg-shell");
      return false;
    }

    m_surface = wl_compositor_create_surface(m_compositor);
    m_xdgSurface = xdg_wm_base_get_xdg_surface(m_shell, m_surface);
    xdg_surface_add_listener(m_xdgSurface, &kXdgListener, this);
    m_toplevel = xdg_surface_get_toplevel(m_xdgSurface);
    xdg_toplevel_add_listener(m_toplevel, &kTopListener, this);
    xdg_toplevel_set_title(m_toplevel, "noctalia-greeter debug");
    wl_surface_commit(m_surface);
    while (!m_configured && wl_display_dispatch(m_display) >= 0) {
    }

    m_eglDisplay = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(m_display));
    EGLint major = 0;
    EGLint minor = 0;
    if (eglInitialize(m_eglDisplay, &major, &minor) != EGL_TRUE || eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
      throw std::runtime_error("failed to initialize EGL");
    }
    EGLint count = 0;
    eglChooseConfig(m_eglDisplay, kConfigAttrs, &m_config, 1, &count);
    m_context = eglCreateContext(m_eglDisplay, m_config, EGL_NO_CONTEXT, kContextAttrs);
    m_window = wl_egl_window_create(m_surface, m_width, m_height);
    m_eglSurface =
        eglCreateWindowSurface(m_eglDisplay, m_config, reinterpret_cast<EGLNativeWindowType>(m_window), nullptr);
    eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_context);
    eglSwapInterval(m_eglDisplay, 0);
    m_renderer.initialize();
    m_xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    core::log(core::LogLevel::Info, "wayland", "debug Wayland/EGL platform ready");
    return true;
  }

  int DebugWaylandPlatform::run(ui::LoginView& view, std::function<void(const ui::LoginRequest&)> submit) {
    m_view = &view;
    while (m_running) {
      while (wl_display_prepare_read(m_display) != 0) {
        wl_display_dispatch_pending(m_display);
      }
      wl_display_flush(m_display);
      wl_display_cancel_read(m_display);
      wl_display_dispatch_pending(m_display);

      eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_context);
      if (auto request = view.takeSubmit()) {
        submit(*request);
      }
      processKeyRepeat();
      view.render(m_renderer, m_width, m_height);
      eglSwapBuffers(m_eglDisplay, m_eglSurface);
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    return 0;
  }

  void DebugWaylandPlatform::global(void* data, wl_registry* registry, uint32_t name, const char* interface,
                                    uint32_t version) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
      self->m_compositor = static_cast<wl_compositor*>(
          wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
      self->m_seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 5u)));
      wl_seat_add_listener(self->m_seat, &kSeatListener, self);
    } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
      self->m_shell = static_cast<xdg_wm_base*>(wl_registry_bind(registry, name, &xdg_wm_base_interface, 1));
      xdg_wm_base_add_listener(self->m_shell, &kShellListener, self);
    }
  }

  void DebugWaylandPlatform::ping(void*, xdg_wm_base* shell, uint32_t serial) { xdg_wm_base_pong(shell, serial); }

  void DebugWaylandPlatform::xdgConfigure(void* data, xdg_surface* surface, uint32_t serial) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    xdg_surface_ack_configure(surface, serial);
    self->m_configured = true;
  }

  void DebugWaylandPlatform::topConfigure(void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array*) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (width > 0 && height > 0) {
      self->m_width = width;
      self->m_height = height;
      if (self->m_window != nullptr) {
        wl_egl_window_resize(self->m_window, width, height, 0, 0);
      }
    }
  }

  void DebugWaylandPlatform::topClose(void* data, xdg_toplevel*) {
    static_cast<DebugWaylandPlatform*>(data)->m_running = false;
  }

  void DebugWaylandPlatform::seatCapabilities(void* data, wl_seat* seat, uint32_t capabilities) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) != 0 && self->m_pointer == nullptr) {
      self->m_pointer = wl_seat_get_pointer(seat);
      wl_pointer_add_listener(self->m_pointer, &kPointerListener, self);
    } else if ((capabilities & WL_SEAT_CAPABILITY_POINTER) == 0 && self->m_pointer != nullptr) {
      wl_pointer_destroy(self->m_pointer);
      self->m_pointer = nullptr;
    }
    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0 && self->m_keyboard == nullptr) {
      self->m_keyboard = wl_seat_get_keyboard(seat);
      wl_keyboard_add_listener(self->m_keyboard, &kKeyboardListener, self);
    } else if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) == 0 && self->m_keyboard != nullptr) {
      wl_keyboard_destroy(self->m_keyboard);
      self->m_keyboard = nullptr;
    }
  }

  void DebugWaylandPlatform::pointerEnter(void* data, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t sx,
                                          wl_fixed_t sy) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (self->m_view != nullptr) {
      self->m_view->pointerMove(static_cast<float>(wl_fixed_to_double(sx)), static_cast<float>(wl_fixed_to_double(sy)));
    }
  }

  void DebugWaylandPlatform::pointerLeave(void* data, wl_pointer*, uint32_t, wl_surface*) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (self->m_view != nullptr) {
      self->m_view->pointerMove(-1.0f, -1.0f);
    }
  }

  void DebugWaylandPlatform::pointerMotion(void* data, wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (self->m_view != nullptr) {
      self->m_view->pointerMove(static_cast<float>(wl_fixed_to_double(sx)), static_cast<float>(wl_fixed_to_double(sy)));
    }
  }

  void DebugWaylandPlatform::pointerButton(void* data, wl_pointer*, uint32_t, uint32_t, uint32_t button,
                                           uint32_t state) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (self->m_view != nullptr && button == BTN_LEFT) {
      self->m_view->pointerButton(state == WL_POINTER_BUTTON_STATE_PRESSED);
    }
  }

  void DebugWaylandPlatform::keyboardKeymap(void* data, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1 || self->m_xkbContext == nullptr) {
      ::close(fd);
      return;
    }

    void* mapped = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED) {
      ::close(fd);
      return;
    }

    auto* keymap = xkb_keymap_new_from_string(self->m_xkbContext, static_cast<const char*>(mapped),
                                              XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(mapped, size);
    ::close(fd);
    if (keymap == nullptr) {
      return;
    }

    if (self->m_xkbState != nullptr) {
      xkb_state_unref(self->m_xkbState);
    }
    if (self->m_xkbKeymap != nullptr) {
      xkb_keymap_unref(self->m_xkbKeymap);
    }
    self->m_xkbKeymap = keymap;
    self->m_xkbState = xkb_state_new(self->m_xkbKeymap);
  }

  void DebugWaylandPlatform::keyboardKey(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (self->m_view == nullptr || self->m_xkbState == nullptr) {
      return;
    }

    const xkb_keycode_t code = key + 8;
    if (state != WL_KEYBOARD_KEY_STATE_PRESSED) {
      if (self->m_repeatingKey && self->m_repeatingKey->waylandKey == key) {
        self->m_repeatingKey.reset();
      }
      return;
    }

    self->dispatchKey(code);
    const xkb_keysym_t sym = xkb_state_key_get_one_sym(self->m_xkbState, code);
    const std::uint32_t utf32 = xkb_state_key_get_utf32(self->m_xkbState, code);
    if (self->repeatableKey(sym, utf32) && self->m_repeatRate > 0) {
      self->m_repeatingKey = RepeatingKey{
          .waylandKey = key,
          .code = code,
          .sym = sym,
          .utf32 = utf32,
          .modifiers = self->activeModifiers(),
          .nextRepeat = std::chrono::steady_clock::now() + std::chrono::milliseconds(self->m_repeatDelay),
      };
    } else {
      self->m_repeatingKey.reset();
    }
  }

  void DebugWaylandPlatform::keyboardModifiers(void* data, wl_keyboard*, uint32_t, uint32_t depressed, uint32_t latched,
                                               uint32_t locked, uint32_t group) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    if (self->m_xkbState != nullptr) {
      xkb_state_update_mask(self->m_xkbState, depressed, latched, locked, 0, 0, group);
    }
  }

  void DebugWaylandPlatform::keyboardRepeatInfo(void* data, wl_keyboard*, int32_t rate, int32_t delay) {
    auto* self = static_cast<DebugWaylandPlatform*>(data);
    self->m_repeatRate = rate;
    self->m_repeatDelay = delay;
    if (rate <= 0) {
      self->m_repeatingKey.reset();
    }
  }

  void DebugWaylandPlatform::dispatchKey(xkb_keycode_t code) {
    if (m_view == nullptr || m_xkbState == nullptr) {
      return;
    }
    const xkb_keysym_t sym = xkb_state_key_get_one_sym(m_xkbState, code);
    const std::uint32_t utf32 = xkb_state_key_get_utf32(m_xkbState, code);
    m_view->keyInput(sym, utf32, activeModifiers());
  }

  void DebugWaylandPlatform::processKeyRepeat() {
    if (!m_repeatingKey || m_repeatRate <= 0 || m_view == nullptr) {
      return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (now < m_repeatingKey->nextRepeat) {
      return;
    }

    const auto interval = std::chrono::microseconds(1000000 / std::max(1, m_repeatRate));
    do {
      m_view->keyInput(m_repeatingKey->sym, m_repeatingKey->utf32, m_repeatingKey->modifiers);
      m_repeatingKey->nextRepeat += interval;
    } while (m_repeatingKey->nextRepeat <= now);
  }

  std::uint32_t DebugWaylandPlatform::activeModifiers() const {
    if (m_xkbState == nullptr) {
      return 0;
    }
    std::uint32_t modifiers = 0;
    if (xkb_state_mod_name_is_active(m_xkbState, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE) > 0) {
      modifiers |= ui::KeyModifierShift;
    }
    if (xkb_state_mod_name_is_active(m_xkbState, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0) {
      modifiers |= ui::KeyModifierCtrl;
    }
    if (xkb_state_mod_name_is_active(m_xkbState, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE) > 0) {
      modifiers |= ui::KeyModifierAlt;
    }
    if (xkb_state_mod_name_is_active(m_xkbState, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE) > 0) {
      modifiers |= ui::KeyModifierSuper;
    }
    return modifiers;
  }

  bool DebugWaylandPlatform::repeatableKey(xkb_keysym_t sym, std::uint32_t utf32) const noexcept {
    if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter || sym == XKB_KEY_Tab) {
      return false;
    }
    return utf32 >= 0x20U || sym == XKB_KEY_BackSpace || sym == XKB_KEY_Delete || sym == XKB_KEY_Left ||
           sym == XKB_KEY_Right || sym == XKB_KEY_Home || sym == XKB_KEY_End;
  }

  void DebugWaylandPlatform::cleanup() {
    if (m_eglDisplay != EGL_NO_DISPLAY) {
      eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (m_eglSurface != EGL_NO_SURFACE) {
      eglDestroySurface(m_eglDisplay, m_eglSurface);
    }
    if (m_context != EGL_NO_CONTEXT) {
      eglDestroyContext(m_eglDisplay, m_context);
    }
    if (m_eglDisplay != EGL_NO_DISPLAY) {
      eglTerminate(m_eglDisplay);
    }
    if (m_window != nullptr) {
      wl_egl_window_destroy(m_window);
    }
    if (m_toplevel != nullptr) {
      xdg_toplevel_destroy(m_toplevel);
    }
    if (m_xdgSurface != nullptr) {
      xdg_surface_destroy(m_xdgSurface);
    }
    if (m_surface != nullptr) {
      wl_surface_destroy(m_surface);
    }
    if (m_shell != nullptr) {
      xdg_wm_base_destroy(m_shell);
    }
    if (m_keyboard != nullptr) {
      wl_keyboard_destroy(m_keyboard);
    }
    if (m_pointer != nullptr) {
      wl_pointer_destroy(m_pointer);
    }
    if (m_seat != nullptr) {
      wl_seat_destroy(m_seat);
    }
    if (m_compositor != nullptr) {
      wl_compositor_destroy(m_compositor);
    }
    if (m_registry != nullptr) {
      wl_registry_destroy(m_registry);
    }
    if (m_display != nullptr) {
      wl_display_disconnect(m_display);
    }
    if (m_xkbState != nullptr) {
      xkb_state_unref(m_xkbState);
    }
    if (m_xkbKeymap != nullptr) {
      xkb_keymap_unref(m_xkbKeymap);
    }
    if (m_xkbContext != nullptr) {
      xkb_context_unref(m_xkbContext);
    }
  }

} // namespace noctalia::platform
