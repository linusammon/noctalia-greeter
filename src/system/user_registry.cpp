#include "system/user_registry.h"

#include <algorithm>
#include <pwd.h>
#include <string>

namespace noctalia::system {

  std::vector<User> enumerateUsers() {
    std::vector<User> users;
    setpwent();
    while (passwd* pw = getpwent()) {
      if (pw->pw_uid < 1000 || pw->pw_shell == nullptr ||
          std::string(pw->pw_shell).find("nologin") != std::string::npos) {
        continue;
      }
      std::string gecos = pw->pw_gecos != nullptr ? pw->pw_gecos : "";
      if (const auto comma = gecos.find(','); comma != std::string::npos) {
        gecos.resize(comma);
      }
      users.push_back(User{.name = pw->pw_name != nullptr ? pw->pw_name : "",
                           .realName = gecos,
                           .home = pw->pw_dir != nullptr ? pw->pw_dir : "",
                           .shell = pw->pw_shell != nullptr ? pw->pw_shell : "",
                           .uid = pw->pw_uid,
                           .gid = pw->pw_gid});
    }
    endpwent();
    std::sort(users.begin(), users.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
    return users;
  }

} // namespace noctalia::system
