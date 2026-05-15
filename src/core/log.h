#pragma once

#include <string_view>

namespace noctalia::core {

  enum class LogLevel { Debug, Info, Warn, Error };

  void setDebugLogging(bool enabled);
  void log(LogLevel level, std::string_view scope, std::string_view message);

} // namespace noctalia::core
