#include "platform/drm_platform.h"

#include "core/log.h"

namespace noctalia::platform {

  bool DrmPlatform::initialize() {
    core::log(core::LogLevel::Warn, "drm", "DRM/KMS platform scaffold is present but not yet bound to GBM scanout");
    core::log(core::LogLevel::Warn, "drm", "run with --debug for the Wayland/EGL development backend");
    return false;
  }

  int DrmPlatform::run(ui::LoginView&, std::function<void(const ui::LoginRequest&)>) { return 1; }

} // namespace noctalia::platform
