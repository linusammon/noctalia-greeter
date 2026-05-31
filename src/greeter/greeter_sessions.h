#pragma once

#include <string>
#include <vector>

namespace greeter {

struct SessionOption {
  std::string name;
  std::string command;
};

[[nodiscard]] std::vector<SessionOption> discoverSessions();

} // namespace greeter
