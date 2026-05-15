#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace noctalia::session {

  enum class SessionType { X11, Wayland };

  struct SessionSearchDir {
    std::filesystem::path path;
    SessionType type = SessionType::Wayland;
  };

  struct Session {
    std::string id;
    std::string name;
    std::string exec;
    std::filesystem::path desktopFile;
    SessionType type = SessionType::Wayland;
  };

  std::vector<Session> discoverSessions();
  std::vector<Session> discoverSessionsFromDirs(const std::vector<SessionSearchDir>& dirs);

} // namespace noctalia::session
