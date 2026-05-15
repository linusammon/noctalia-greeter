#pragma once

#include "session/session_registry.h"
#include "system/user_registry.h"

#include <string>
#include <vector>

namespace noctalia::session {

  class SessionLauncher {
  public:
    int launch(const system::User& user, const Session& session, const std::vector<std::string>& pamEnvironment,
               bool debugMode);
  };

} // namespace noctalia::session
