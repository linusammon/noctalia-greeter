#include "ui/login_view.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace noctalia::ui {
  namespace {
    using render::Color;

    constexpr Color kPrimary{1.0f, 0.961f, 0.608f, 1.0f};
    constexpr Color kOnPrimary{0.055f, 0.055f, 0.263f, 1.0f};
    constexpr Color kSecondary{0.663f, 0.682f, 0.996f, 1.0f};
    constexpr Color kTertiary{0.608f, 0.996f, 0.808f, 1.0f};
    constexpr Color kError{0.992f, 0.275f, 0.388f, 1.0f};
    constexpr Color kSurface{0.027f, 0.027f, 0.133f, 1.0f};
    constexpr Color kOnSurface{0.953f, 0.929f, 0.969f, 1.0f};
    constexpr Color kSurfaceVariant{0.067f, 0.067f, 0.176f, 1.0f};
    constexpr Color kOnSurfaceVariant{0.486f, 0.502f, 0.706f, 1.0f};
    constexpr Color kOutline{0.129f, 0.129f, 0.373f, 1.0f};

    Color withAlpha(Color color, float alpha) {
      color.a *= alpha;
      return color;
    }

    std::string_view passwordGlyphForIndex(std::size_t index) {
      static constexpr std::array<std::string_view, 7> kGlyphs = {
          "circle-filled",      "pentagon-filled", "michelin-star-filled", "square-rounded-filled",
          "guitar-pick-filled", "blob-filled",     "triangle-filled",
      };
      return kGlyphs[index % kGlyphs.size()];
    }

  } // namespace

  LoginView::LoginView() {
    m_passwordInput.setPlaceholder("Type password");
    m_passwordInput.setPasswordMode(true);
  }

  void LoginView::setData(std::vector<system::User> users, std::vector<session::Session> sessions) {
    m_users = std::move(users);
    m_sessions = std::move(sessions);
    m_userIndex = 0;
    m_sessionIndex = 0;
  }

  void LoginView::setStatus(std::string status, bool authenticated) {
    m_status = std::move(status);
    m_authenticated = authenticated;
  }

  void LoginView::render(render::GlRenderer& renderer, int width, int height) {
    renderer.beginFrame(width, height);

    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    renderer.drawRect(0.0f, 0.0f, w, h, 0.0f, kSurface);
    renderer.drawRect(24.0f, 24.0f, w - 48.0f, h - 48.0f, 22.0f, withAlpha(kSurfaceVariant, 0.28f),
                      withAlpha(kOutline, 0.45f), 1.0f);

    const float panelW = std::min(760.0f, w - 72.0f);
    const float panelH = 440.0f;
    const float x = std::round((w - panelW) * 0.5f);
    const float y = std::round((h - panelH) * 0.5f);
    const float leftW = std::max(220.0f, panelW * 0.38f);
    const float formX = x + leftW + 28.0f;
    const float formW = panelW - leftW - 60.0f;

    renderer.drawRect(x, y, panelW, panelH, 18.0f, withAlpha(kSurfaceVariant, 0.94f), withAlpha(kOutline, 0.90f), 1.0f);
    renderer.drawRect(x + 24.0f, y + 24.0f, leftW - 48.0f, panelH - 48.0f, 14.0f, withAlpha(kSurface, 0.54f),
                      withAlpha(kOutline, 0.55f), 1.0f);
    renderer.drawRect(x + 48.0f, y + 50.0f, 72.0f, 72.0f, 20.0f, withAlpha(kPrimary, 0.16f), kPrimary, 1.0f);
    renderer.drawNoctaliaLogo(x + 56.0f, y + 58.0f, 56.0f);
    renderer.drawText(x + 48.0f, y + 154.0f, "Noctalia", leftW - 84.0f, 34, kOnSurface);
    renderer.drawText(x + 50.0f, y + 196.0f, "Direct session login", leftW - 88.0f, 16, kOnSurfaceVariant);
    renderer.drawRect(x + 48.0f, y + panelH - 96.0f, leftW - 88.0f, 1.0f, 0.0f, withAlpha(kOutline, 0.9f));
    renderer.drawGlyph(x + 48.0f, y + panelH - 72.0f, "shield-lock", 22, kTertiary);
    renderer.drawText(x + 82.0f, y + panelH - 70.0f, "PAM authenticated", leftW - 122.0f, 15, kOnSurfaceVariant);

    renderer.drawText(formX, y + 42.0f, "Sign in", formW, 28, kOnSurface);
    renderer.drawText(formX, y + 76.0f, "Choose a user, pick a desktop session, and unlock.", formW, 15,
                      kOnSurfaceVariant);

    m_userRect = Rect{formX, y + 126.0f, formW, 50.0f};
    m_sessionRect = Rect{formX, y + 190.0f, formW, 50.0f};
    m_passwordRect = Rect{formX, y + 254.0f, formW, 50.0f};
    m_submitRect = Rect{formX, y + 328.0f, formW, 52.0f};

    const std::string userName = m_userIndex < m_users.size() ? m_users[m_userIndex].name : "No users";
    const std::string sessionName =
        m_sessionIndex < m_sessions.size() ? m_sessions[m_sessionIndex].name : "No sessions";
    drawSelect(renderer, m_userRect, "User", userName, m_userOpen, m_focus == Field::User);
    drawSelect(renderer, m_sessionRect, "Session", sessionName, m_sessionOpen, m_focus == Field::Session);
    drawInput(renderer, m_passwordRect);

    const bool submitHover = m_submitRect.contains(m_pointerX, m_pointerY);
    renderer.drawRect(m_submitRect.x, m_submitRect.y, m_submitRect.w, m_submitRect.h, 9.0f,
                      submitHover ? kTertiary : kPrimary, withAlpha(kOutline, 0.65f), 1.0f);
    renderer.drawText(m_submitRect.x + 24.0f, m_submitRect.y + 15.0f, "Sign in", m_submitRect.w - 78.0f, 17,
                      kOnPrimary);
    renderer.drawGlyph(m_submitRect.x + m_submitRect.w - 42.0f, m_submitRect.y + 15.0f, "login", 22, kOnPrimary);

    const Color statusColor =
        m_authenticated ? kTertiary : (m_status.find("failed") != std::string::npos ? kError : kOnSurfaceVariant);
    renderer.drawText(formX, y + panelH - 36.0f, m_status, formW, 15, statusColor);

    drawDropdown(renderer, m_userRect, "User", m_userOpen);
    drawDropdown(renderer, m_sessionRect, "Session", m_sessionOpen);
  }

  void LoginView::drawSelect(render::GlRenderer& renderer, const Rect& rect, std::string_view label,
                             std::string_view value, bool open, bool active) {
    const bool hover = rect.contains(m_pointerX, m_pointerY);
    renderer.drawRect(rect.x, rect.y, rect.w, rect.h, 8.0f,
                      hover ? withAlpha(kOutline, 0.58f) : withAlpha(kSurface, 0.92f), active ? kPrimary : kOutline,
                      active ? 1.5f : 1.0f);
    const char* icon = label == "User" ? "user" : "device-desktop";
    renderer.drawGlyph(rect.x + 14.0f, rect.y + 15.0f, icon, 21, active ? kPrimary : kSecondary);
    renderer.drawText(rect.x + 48.0f, rect.y + 7.0f, std::string(label), rect.w - 92.0f, 12, kOnSurfaceVariant);
    renderer.drawText(rect.x + 48.0f, rect.y + 25.0f, std::string(value), rect.w - 92.0f, 16, kOnSurface);
    renderer.drawGlyph(rect.x + rect.w - 34.0f, rect.y + 16.0f, open ? "chevron-up" : "chevron-down", 20, kSecondary);

    auto& optionRects = label == "User" ? m_userOptionRects : m_sessionOptionRects;
    optionRects.clear();
  }

  void LoginView::drawDropdown(render::GlRenderer& renderer, const Rect& owner, std::string_view label, bool open) {
    if (!open) {
      return;
    }

    const auto count = label == "User" ? m_users.size() : m_sessions.size();
    const std::size_t visible = std::min<std::size_t>(count, 6);
    const float optionH = 42.0f;
    const float menuY = owner.y + owner.h + 8.0f;
    auto& optionRects = label == "User" ? m_userOptionRects : m_sessionOptionRects;
    optionRects.clear();
    renderer.drawRect(owner.x, menuY, owner.w, optionH * static_cast<float>(visible), 10.0f, kSurfaceVariant, kOutline,
                      1.0f);
    for (std::size_t i = 0; i < visible; ++i) {
      Rect option{owner.x, menuY + static_cast<float>(i) * optionH, owner.w, optionH};
      optionRects.push_back(option);
      const bool optionHover = option.contains(m_pointerX, m_pointerY);
      if (optionHover) {
        renderer.drawRect(option.x + 4.0f, option.y + 4.0f, option.w - 8.0f, option.h - 8.0f, 7.0f,
                          withAlpha(kOutline, 0.65f));
      }
      const std::string text = label == "User" ? m_users[i].name : m_sessions[i].name;
      renderer.drawText(option.x + 18.0f, option.y + 12.0f, text, option.w - 36.0f, 15, kOnSurface);
    }
  }

  void LoginView::drawInput(render::GlRenderer& renderer, const Rect& rect) {
    const bool hover = rect.contains(m_pointerX, m_pointerY);
    const bool active = m_focus == Field::Password;
    m_passwordInput.setFocused(active);
    renderer.drawRect(rect.x, rect.y, rect.w, rect.h, 8.0f,
                      hover ? withAlpha(kOutline, 0.58f) : withAlpha(kSurface, 0.92f), active ? kPrimary : kOutline,
                      active ? 1.5f : 1.0f);
    renderer.drawGlyph(rect.x + 14.0f, rect.y + 15.0f, "lock", 21, active ? kPrimary : kSecondary);
    renderer.drawText(rect.x + 48.0f, rect.y + 7.0f, "Password", rect.w - 64.0f, 12, kOnSurfaceVariant);
    const bool empty = m_passwordInput.empty();
    if (empty) {
      renderer.drawText(rect.x + 48.0f, rect.y + 25.0f, m_passwordInput.placeholder(), rect.w - 64.0f, 16,
                        kOnSurfaceVariant);
      return;
    }

    const float glyphSize = 14.0f;
    const float glyphStep = 17.0f;
    const float glyphX = rect.x + 48.0f;
    const float glyphY = rect.y + 27.0f;
    const auto selectionStart = m_passwordInput.selectionStartGlyphIndex();
    const auto selectionEnd = m_passwordInput.selectionEndGlyphIndex();
    if (selectionEnd > selectionStart) {
      renderer.drawRect(glyphX + static_cast<float>(selectionStart) * glyphStep - 3.0f, rect.y + 23.0f,
                        static_cast<float>(selectionEnd - selectionStart) * glyphStep + 6.0f, 20.0f, 4.0f,
                        withAlpha(kPrimary, 0.28f));
    }
    const auto glyphCount = m_passwordInput.glyphCount();
    const float maxGlyphs = std::max(0.0f, std::floor((rect.w - 84.0f) / glyphStep));
    const auto visibleGlyphs = std::min<std::size_t>(glyphCount, static_cast<std::size_t>(maxGlyphs));
    for (std::size_t i = 0; i < visibleGlyphs; ++i) {
      renderer.drawGlyph(glyphX + static_cast<float>(i) * glyphStep, glyphY, passwordGlyphForIndex(i), glyphSize,
                         kOnSurface);
    }
    if (active) {
      const auto cursor = std::min<std::size_t>(m_passwordInput.cursorGlyphIndex(), visibleGlyphs);
      renderer.drawRect(glyphX + static_cast<float>(cursor) * glyphStep - 1.0f, rect.y + 24.0f, 1.5f, 20.0f, 1.0f,
                        kPrimary);
    }
  }

  void LoginView::pointerMove(float x, float y) {
    m_pointerX = x;
    m_pointerY = y;
  }

  void LoginView::pointerButton(bool pressed) {
    if (!pressed) {
      return;
    }

    for (std::size_t i = 0; i < m_userOptionRects.size(); ++i) {
      if (m_userOptionRects[i].contains(m_pointerX, m_pointerY)) {
        m_userIndex = i;
        m_userOpen = false;
        m_focus = Field::User;
        return;
      }
    }
    for (std::size_t i = 0; i < m_sessionOptionRects.size(); ++i) {
      if (m_sessionOptionRects[i].contains(m_pointerX, m_pointerY)) {
        m_sessionIndex = i;
        m_sessionOpen = false;
        m_focus = Field::Session;
        return;
      }
    }

    if (m_userRect.contains(m_pointerX, m_pointerY)) {
      m_userOpen = !m_userOpen;
      m_sessionOpen = false;
      m_focus = Field::User;
    } else if (m_sessionRect.contains(m_pointerX, m_pointerY)) {
      m_sessionOpen = !m_sessionOpen;
      m_userOpen = false;
      m_focus = Field::Session;
    } else if (m_passwordRect.contains(m_pointerX, m_pointerY)) {
      m_userOpen = false;
      m_sessionOpen = false;
      m_focus = Field::Password;
    } else if (m_submitRect.contains(m_pointerX, m_pointerY)) {
      submit();
    } else {
      m_userOpen = false;
      m_sessionOpen = false;
    }
  }

  void LoginView::textInput(std::string_view text) {
    if (m_focus == Field::Password) {
      m_passwordInput.insertText(text);
    }
  }

  void LoginView::keyInput(std::uint32_t sym, std::uint32_t utf32, std::uint32_t modifiers) {
    if (sym == XKB_KEY_Return || sym == XKB_KEY_KP_Enter) {
      keyReturn();
      return;
    }
    if (sym == XKB_KEY_Tab) {
      keyTab();
      return;
    }
    if (m_focus == Field::Password) {
      (void)m_passwordInput.handleKey(sym, utf32, modifiers);
    }
  }

  void LoginView::keyBackspace() {
    if (m_focus == Field::Password) {
      m_passwordInput.backspace();
    }
  }

  void LoginView::keyReturn() {
    if (m_userOpen) {
      m_userOpen = false;
    } else if (m_sessionOpen) {
      m_sessionOpen = false;
    } else {
      submit();
    }
  }

  void LoginView::keyTab() {
    m_userOpen = false;
    m_sessionOpen = false;
    if (m_focus == Field::User) {
      m_focus = Field::Session;
    } else if (m_focus == Field::Session) {
      m_focus = Field::Password;
    } else if (m_focus == Field::Password) {
      m_focus = Field::Submit;
    } else {
      m_focus = Field::User;
    }
  }

  void LoginView::submit() {
    if (m_users.empty() || m_sessions.empty()) {
      m_status = "No users or sessions are available";
      return;
    }
    m_status = "Authenticating";
    m_submit = LoginRequest{
        .user = m_users[m_userIndex].name,
        .password = m_passwordInput.value(),
        .sessionId = m_sessions[m_sessionIndex].id,
    };
  }

  std::optional<LoginRequest> LoginView::takeSubmit() {
    auto request = std::move(m_submit);
    m_submit.reset();
    return request;
  }

} // namespace noctalia::ui
