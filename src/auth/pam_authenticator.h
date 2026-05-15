#pragma once

#include <string>
#include <vector>

namespace noctalia::auth {

  struct PamResult {
    bool ok = false;
    std::string error;
    std::vector<std::string> environment;
  };

  class PamAuthenticator {
  public:
    PamResult authenticate(const std::string& user, const std::string& password,
                           const char* service = "noctalia-greeter");
  };

} // namespace noctalia::auth
