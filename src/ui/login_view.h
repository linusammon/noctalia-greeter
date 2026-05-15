#pragma once

#include "render/gl_renderer.h"
#include "session/session_registry.h"
#include "system/user_registry.h"
#include "ui/controls/input.h"

#include <optional>
#include <string>
#include <vector>

namespace noctalia::ui {

  struct LoginRequest {
    std::string user;
    std::string password;
    std::string sessionId;
  };

  class LoginView {
  public:
    LoginView();

    void setData(std::vector<system::User> users, std::vector<session::Session> sessions);
    void setStatus(std::string status, bool authenticated = false);
    void render(render::GlRenderer& renderer, int width, int height);

    void pointerMove(float x, float y);
    void pointerButton(bool pressed);
    void keyInput(std::uint32_t sym, std::uint32_t utf32, std::uint32_t modifiers);
    void textInput(std::string_view text);
    void keyBackspace();
    void keyReturn();
    void keyTab();

    [[nodiscard]] std::optional<LoginRequest> takeSubmit();

  private:
    enum class Field { User, Session, Password, Submit };

    struct Rect {
      float x = 0.0f;
      float y = 0.0f;
      float w = 0.0f;
      float h = 0.0f;
      [[nodiscard]] bool contains(float px, float py) const { return px >= x && px <= x + w && py >= y && py <= y + h; }
    };

    void submit();
    void drawSelect(render::GlRenderer& renderer, const Rect& rect, std::string_view label, std::string_view value,
                    bool open, bool active);
    void drawDropdown(render::GlRenderer& renderer, const Rect& owner, std::string_view label, bool open);
    void drawInput(render::GlRenderer& renderer, const Rect& rect);

    std::vector<system::User> m_users;
    std::vector<session::Session> m_sessions;
    std::size_t m_userIndex = 0;
    std::size_t m_sessionIndex = 0;
    Input m_passwordInput;
    std::string m_status = "Choose a user and session";
    bool m_authenticated = false;
    Field m_focus = Field::Password;
    bool m_userOpen = false;
    bool m_sessionOpen = false;
    float m_pointerX = -1.0f;
    float m_pointerY = -1.0f;
    Rect m_userRect;
    Rect m_sessionRect;
    Rect m_passwordRect;
    Rect m_submitRect;
    std::vector<Rect> m_userOptionRects;
    std::vector<Rect> m_sessionOptionRects;
    std::optional<LoginRequest> m_submit;
  };

} // namespace noctalia::ui
