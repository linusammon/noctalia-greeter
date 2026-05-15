#include "core/resource_paths.h"

#include <array>

namespace noctalia::core {

  std::filesystem::path assetPath(const std::filesystem::path& relative) {
    const std::array<std::filesystem::path, 3> roots{
        std::filesystem::current_path() / "assets",
        std::filesystem::current_path() / ".." / "assets",
        std::filesystem::path(NOCTALIA_GREETER_DATADIR),
    };

    for (const auto& root : roots) {
      std::error_code ec;
      auto candidate = root / relative;
      if (std::filesystem::exists(candidate, ec)) {
        return candidate;
      }
    }

    return roots.front() / relative;
  }

} // namespace noctalia::core
