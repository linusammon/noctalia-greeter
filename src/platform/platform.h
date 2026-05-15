#pragma once

#include "render/gl_renderer.h"
#include "ui/login_view.h"

#include <functional>
#include <string>

namespace noctalia::platform {

  struct FrameState {
    bool authenticated = false;
    std::string status;
  };

  class Platform {
  public:
    virtual ~Platform() = default;
    virtual bool initialize() = 0;
    virtual int run(ui::LoginView& view, std::function<void(const ui::LoginRequest&)> submit) = 0;
  };

} // namespace noctalia::platform
