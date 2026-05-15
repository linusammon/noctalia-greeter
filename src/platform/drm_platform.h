#pragma once

#include "platform/platform.h"

namespace noctalia::platform {

  class DrmPlatform final : public Platform {
  public:
    bool initialize() override;
    int run(ui::LoginView& view, std::function<void(const ui::LoginRequest&)> submit) override;

  private:
    render::GlRenderer m_renderer;
  };

} // namespace noctalia::platform
