#include "session/session_registry.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {

  void writeFile(const std::filesystem::path& path, const std::string& text) {
    std::ofstream file(path);
    file << text;
  }

  bool expect(bool condition, const char* message) {
    if (!condition) {
      std::cerr << "FAIL: " << message << '\n';
      return false;
    }
    return true;
  }

} // namespace

int main() {
  const auto root =
      std::filesystem::temp_directory_path() / ("noctalia-greeter-session-test-" + std::to_string(getpid()));
  const auto wayland = root / "wayland-sessions";
  const auto x11 = root / "xsessions";
  std::filesystem::create_directories(wayland);
  std::filesystem::create_directories(x11);

  writeFile(wayland / "mango.desktop", "[Desktop Entry]\n"
                                       "Name=Mango\n"
                                       "Exec=mango-session\n");
  writeFile(x11 / "dwl.desktop", "[Desktop Entry]\n"
                                 "Name=dwl\n"
                                 "Exec=dwl\n");
  writeFile(wayland / "hidden.desktop", "[Desktop Entry]\n"
                                        "Name=Hidden\n"
                                        "Exec=hidden-session\n"
                                        "Hidden=true\n");
  writeFile(x11 / "nodisplay.desktop", "[Desktop Entry]\n"
                                       "Name=No Display\n"
                                       "Exec=no-display\n"
                                       "NoDisplay=true\n");
  writeFile(x11 / "missing-exec.desktop", "[Desktop Entry]\n"
                                          "Name=Missing Exec\n");

  const auto sessions = noctalia::session::discoverSessionsFromDirs({
      {wayland, noctalia::session::SessionType::Wayland},
      {x11, noctalia::session::SessionType::X11},
  });

  bool ok = true;
  ok &= expect(sessions.size() == 2, "only visible sessions with Exec are returned");
  ok &= expect(sessions[0].name == "Mango", "sessions are sorted by name");
  ok &= expect(sessions[0].type == noctalia::session::SessionType::Wayland, "wayland session type is preserved");
  ok &= expect(sessions[1].id == "dwl", "desktop file stem becomes id");
  ok &= expect(sessions[1].exec == "dwl", "Exec value is parsed");

  std::filesystem::remove_all(root);
  return ok ? 0 : 1;
}
