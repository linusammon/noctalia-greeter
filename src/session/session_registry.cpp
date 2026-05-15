#include "session/session_registry.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace noctalia::session {
  namespace {
    std::string trim(std::string value) {
      const auto first = value.find_first_not_of(" \t\r\n");
      const auto last = value.find_last_not_of(" \t\r\n");
      if (first == std::string::npos) {
        return {};
      }
      return value.substr(first, last - first + 1);
    }

    void scanDir(const std::filesystem::path& dir, SessionType type, std::vector<Session>& out) {
      std::error_code ec;
      if (!std::filesystem::is_directory(dir, ec)) {
        return;
      }
      for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.path().extension() != ".desktop") {
          continue;
        }
        std::ifstream file(entry.path());
        std::string line;
        Session session;
        session.id = entry.path().stem().string();
        session.desktopFile = entry.path();
        session.type = type;
        bool inDesktop = false;
        bool hidden = false;
        bool noDisplay = false;
        while (std::getline(file, line)) {
          line = trim(line);
          if (line.empty() || line[0] == '#') {
            continue;
          }
          if (line.front() == '[' && line.back() == ']') {
            inDesktop = line == "[Desktop Entry]";
            continue;
          }
          if (!inDesktop) {
            continue;
          }
          const auto eq = line.find('=');
          if (eq == std::string::npos) {
            continue;
          }
          const auto key = line.substr(0, eq);
          const auto value = trim(line.substr(eq + 1));
          if (key == "Name") {
            session.name = value;
          } else if (key == "Exec") {
            session.exec = value;
          } else if (key == "Hidden") {
            hidden = value == "true";
          } else if (key == "NoDisplay") {
            noDisplay = value == "true";
          }
        }
        if (!hidden && !noDisplay && !session.exec.empty()) {
          if (session.name.empty()) {
            session.name = session.id;
          }
          out.push_back(std::move(session));
        }
      }
    }
  } // namespace

  std::vector<Session> discoverSessionsFromDirs(const std::vector<SessionSearchDir>& dirs) {
    std::vector<Session> sessions;
    for (const auto& dir : dirs) {
      scanDir(dir.path, dir.type, sessions);
    }
    std::sort(sessions.begin(), sessions.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
    return sessions;
  }

  std::vector<Session> discoverSessions() {
    return discoverSessionsFromDirs({
        {"/usr/share/wayland-sessions", SessionType::Wayland},
        {"/usr/local/share/wayland-sessions", SessionType::Wayland},
        {"/usr/share/xsessions", SessionType::X11},
        {"/usr/local/share/xsessions", SessionType::X11},
    });
  }

} // namespace noctalia::session
