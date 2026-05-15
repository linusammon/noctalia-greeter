#pragma once

#include "platform/platform.h"
#include "xdg-shell-client-protocol.h"

#include <EGL/egl.h>
#include <chrono>
#include <optional>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>

namespace noctalia::platform {

  class DebugWaylandPlatform final : public Platform {
  public:
    ~DebugWaylandPlatform() override;
    bool initialize() override;
    int run(ui::LoginView& view, std::function<void(const ui::LoginRequest&)> submit) override;

    static void global(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version);
    static void globalRemove(void*, wl_registry*, uint32_t) {}
    static void ping(void*, xdg_wm_base* shell, uint32_t serial);
    static void xdgConfigure(void* data, xdg_surface* surface, uint32_t serial);
    static void topConfigure(void* data, xdg_toplevel*, int32_t width, int32_t height, wl_array*);
    static void topClose(void* data, xdg_toplevel*);
    static void seatCapabilities(void* data, wl_seat* seat, uint32_t capabilities);
    static void seatName(void*, wl_seat*, const char*) {}
    static void pointerEnter(void* data, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t sx, wl_fixed_t sy);
    static void pointerLeave(void* data, wl_pointer*, uint32_t, wl_surface*);
    static void pointerMotion(void* data, wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy);
    static void pointerButton(void* data, wl_pointer*, uint32_t, uint32_t, uint32_t button, uint32_t state);
    static void pointerAxis(void*, wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {}
    static void pointerFrame(void*, wl_pointer*) {}
    static void pointerAxisSource(void*, wl_pointer*, uint32_t) {}
    static void pointerAxisStop(void*, wl_pointer*, uint32_t, uint32_t) {}
    static void pointerAxisDiscrete(void*, wl_pointer*, uint32_t, int32_t) {}
    static void pointerAxisValue120(void*, wl_pointer*, uint32_t, int32_t) {}
    static void pointerAxisRelativeDirection(void*, wl_pointer*, uint32_t, uint32_t) {}
    static void keyboardKeymap(void* data, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size);
    static void keyboardEnter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {}
    static void keyboardLeave(void*, wl_keyboard*, uint32_t, wl_surface*) {}
    static void keyboardKey(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state);
    static void keyboardModifiers(void* data, wl_keyboard*, uint32_t, uint32_t depressed, uint32_t latched,
                                  uint32_t locked, uint32_t group);
    static void keyboardRepeatInfo(void* data, wl_keyboard*, int32_t rate, int32_t delay);

  private:
    struct RepeatingKey {
      std::uint32_t waylandKey = 0;
      xkb_keycode_t code = 0;
      xkb_keysym_t sym = 0;
      std::uint32_t utf32 = 0;
      std::uint32_t modifiers = 0;
      std::chrono::steady_clock::time_point nextRepeat;
    };

    void cleanup();
    void dispatchKey(xkb_keycode_t code);
    void processKeyRepeat();
    [[nodiscard]] std::uint32_t activeModifiers() const;
    [[nodiscard]] bool repeatableKey(xkb_keysym_t sym, std::uint32_t utf32) const noexcept;

    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    wl_compositor* m_compositor = nullptr;
    wl_seat* m_seat = nullptr;
    wl_pointer* m_pointer = nullptr;
    wl_keyboard* m_keyboard = nullptr;
    xdg_wm_base* m_shell = nullptr;
    wl_surface* m_surface = nullptr;
    xdg_surface* m_xdgSurface = nullptr;
    xdg_toplevel* m_toplevel = nullptr;
    wl_egl_window* m_window = nullptr;
    EGLDisplay m_eglDisplay = EGL_NO_DISPLAY;
    EGLConfig m_config = nullptr;
    EGLContext m_context = EGL_NO_CONTEXT;
    EGLSurface m_eglSurface = EGL_NO_SURFACE;
    render::GlRenderer m_renderer;
    ui::LoginView* m_view = nullptr;
    xkb_context* m_xkbContext = nullptr;
    xkb_keymap* m_xkbKeymap = nullptr;
    xkb_state* m_xkbState = nullptr;
    std::optional<RepeatingKey> m_repeatingKey;
    int m_repeatRate = 25;
    int m_repeatDelay = 600;
    int m_width = 900;
    int m_height = 560;
    bool m_running = true;
    bool m_configured = false;
  };

} // namespace noctalia::platform
