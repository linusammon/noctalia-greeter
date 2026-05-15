#include "render/glyph_registry.h"

#include "core/resource_paths.h"

#include <charconv>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <unordered_map>

namespace noctalia::render {
  namespace {
    std::optional<char32_t> parseCodepoint(std::string_view value) {
      if (value.size() < 3 || value[0] != 'U' || value[1] != '+') {
        return std::nullopt;
      }
      std::uint32_t codepoint = 0;
      const auto* begin = value.data() + 2;
      const auto* end = value.data() + value.size();
      const auto result = std::from_chars(begin, end, codepoint, 16);
      if (result.ec != std::errc{} || result.ptr != end || codepoint == 0 || codepoint > 0x10FFFF) {
        return std::nullopt;
      }
      return static_cast<char32_t>(codepoint);
    }

    std::unordered_map<std::string, char32_t> loadGlyphs() {
      std::unordered_map<std::string, char32_t> glyphs;
      std::ifstream file(core::assetPath("fonts/tabler.json"));
      std::string line;
      while (std::getline(file, line)) {
        const auto keyBegin = line.find('"');
        if (keyBegin == std::string::npos) {
          continue;
        }
        const auto keyEnd = line.find('"', keyBegin + 1);
        const auto valueBegin = line.find("U+", keyEnd);
        const auto valueEnd = line.find('"', valueBegin);
        if (keyEnd == std::string::npos || valueBegin == std::string::npos || valueEnd == std::string::npos) {
          continue;
        }
        auto parsed = parseCodepoint(std::string_view(line).substr(valueBegin, valueEnd - valueBegin));
        if (parsed) {
          glyphs.emplace(line.substr(keyBegin + 1, keyEnd - keyBegin - 1), *parsed);
        }
      }
      return glyphs;
    }

    const std::unordered_map<std::string, char32_t>& glyphs() {
      static const auto loaded = loadGlyphs();
      return loaded;
    }
  } // namespace

  char32_t glyphCodepoint(std::string_view name) {
    const auto& map = glyphs();
    if (const auto it = map.find(std::string(name)); it != map.end()) {
      return it->second;
    }
    return 0xF292;
  }

  std::string utf8(char32_t codepoint) {
    std::string out;
    if (codepoint <= 0x7F) {
      out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
      out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
      out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
      out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
      out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }
    return out;
  }

} // namespace noctalia::render
