#include "core/log.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace noctalia::core {
  namespace {
    bool g_debug = false;

    const char* label(LogLevel level) {
      switch (level) {
      case LogLevel::Debug:
        return "debug";
      case LogLevel::Info:
        return "info";
      case LogLevel::Warn:
        return "warn";
      case LogLevel::Error:
        return "error";
      }
      return "log";
    }
  } // namespace

  void setDebugLogging(bool enabled) { g_debug = enabled; }

  void log(LogLevel level, std::string_view scope, std::string_view message) {
    if (level == LogLevel::Debug && !g_debug) {
      return;
    }
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
    localtime_r(&now, &tm);
    std::fprintf(stderr, "%02d:%02d:%02d %-5s %-10.*s %.*s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, label(level),
                 static_cast<int>(scope.size()), scope.data(), static_cast<int>(message.size()), message.data());
  }

} // namespace noctalia::core
