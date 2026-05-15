#include "session/session_launcher.h"

#include "core/log.h"

#include <cerrno>
#include <cstring>
#include <format>
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

extern char** environ;

namespace noctalia::session {
  namespace {
    void putEnv(const std::string& value) {
      auto* copy = ::strdup(value.c_str());
      if (copy != nullptr) {
        ::putenv(copy);
      }
    }
  } // namespace

  int SessionLauncher::launch(const system::User& user, const Session& session,
                              const std::vector<std::string>& pamEnvironment, bool debugMode) {
    if (debugMode) {
      core::log(core::LogLevel::Info, "session",
                std::format("--debug: authenticated launch skipped for {}", user.name));
      return 0;
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
      core::log(core::LogLevel::Error, "session", std::format("fork failed: {}", std::strerror(errno)));
      return 1;
    }

    if (pid == 0) {
      if (::initgroups(user.name.c_str(), user.gid) != 0 || ::setgid(user.gid) != 0 || ::setuid(user.uid) != 0) {
        _exit(126);
      }

      ::clearenv();
      putEnv("HOME=" + user.home);
      putEnv("USER=" + user.name);
      putEnv("LOGNAME=" + user.name);
      putEnv("SHELL=" + user.shell);
      putEnv("XDG_SESSION_CLASS=user");
      putEnv(std::string("XDG_SESSION_TYPE=") + (session.type == SessionType::Wayland ? "wayland" : "x11"));
      putEnv("XDG_CURRENT_DESKTOP=" + session.name);
      for (const auto& entry : pamEnvironment) {
        putEnv(entry);
      }

      ::chdir(user.home.c_str());
      const char* shell = user.shell.empty() ? "/bin/sh" : user.shell.c_str();
      ::execl(shell, shell, "-lc", session.exec.c_str(), static_cast<char*>(nullptr));
      _exit(127);
    }

    core::log(core::LogLevel::Info, "session",
              std::format("started {} for {} as pid {}", session.name, user.name, pid));
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
  }

} // namespace noctalia::session
