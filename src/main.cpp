#include "auth/pam_authenticator.h"
#include "core/log.h"
#include "platform/debug_wayland_platform.h"
#include "platform/drm_platform.h"
#include "session/session_launcher.h"
#include "session/session_registry.h"
#include "system/user_registry.h"
#include "system/vt_manager.h"
#include "ui/login_view.h"

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

namespace {

  std::atomic<bool> g_running{true};

  void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
      g_running = false;
    }
  }

  struct Options {
    bool debug = false;
    bool help = false;
  };

  Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
      if (std::strcmp(argv[i], "--debug") == 0) {
        options.debug = true;
      } else if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
        options.help = true;
      } else {
        std::fprintf(stderr, "error: unknown option: %s\n", argv[i]);
        options.help = true;
      }
    }
    return options;
  }

  void printHelp() {
    std::puts("Usage: noctalia-greeter [OPTIONS]\n"
              "\n"
              "Options:\n"
              "  --debug      Run inside the current graphical session; skip VT switch and session handoff\n"
              "  -h, --help   Show this help message");
  }

  template <typename T> const T* firstOrNull(const std::vector<T>& values) {
    return values.empty() ? nullptr : &values.front();
  }

} // namespace

int main(int argc, char** argv) {
  const Options options = parseOptions(argc, argv);
  if (options.help) {
    printHelp();
    return 0;
  }

  noctalia::core::setDebugLogging(options.debug);
  std::signal(SIGINT, signalHandler);
  std::signal(SIGTERM, signalHandler);

  auto users = noctalia::system::enumerateUsers();
  auto sessions = noctalia::session::discoverSessions();
  if (users.empty()) {
    noctalia::core::log(noctalia::core::LogLevel::Error, "main", "no login-capable users found");
    return 1;
  }
  if (sessions.empty()) {
    noctalia::core::log(noctalia::core::LogLevel::Error, "main", "no sessions found in xsessions/wayland-sessions");
    return 1;
  }

  noctalia::system::VtManager vt;
  if (!vt.acquire(options.debug)) {
    return 1;
  }

  noctalia::ui::LoginView view;
  view.setData(users, sessions);

  std::unique_ptr<noctalia::platform::Platform> platform;
  if (options.debug) {
    platform = std::make_unique<noctalia::platform::DebugWaylandPlatform>();
  } else {
    platform = std::make_unique<noctalia::platform::DrmPlatform>();
  }

  if (!platform->initialize()) {
    g_running = false;
  } else {
    platform->run(view, [&](const noctalia::ui::LoginRequest& request) {
      auto userIt = std::find_if(users.begin(), users.end(), [&](const auto& u) { return u.name == request.user; });
      auto sessionIt =
          std::find_if(sessions.begin(), sessions.end(), [&](const auto& s) { return s.id == request.sessionId; });
      if (userIt == users.end() || sessionIt == sessions.end()) {
        view.setStatus("Unknown user or session");
        return;
      }

      noctalia::auth::PamAuthenticator pam;
      auto pamResult = pam.authenticate(userIt->name, request.password);
      if (!pamResult.ok) {
        view.setStatus("Authentication failed: " + pamResult.error);
        return;
      }

      view.setStatus("Authenticated", true);
      noctalia::session::SessionLauncher launcher;
      (void)launcher.launch(*userIt, *sessionIt, pamResult.environment, options.debug);
    });
  }
  return 0;
}
