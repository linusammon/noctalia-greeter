#pragma once

#include <string_view>

namespace noctalia::render {

  char32_t glyphCodepoint(std::string_view name);
  std::string utf8(char32_t codepoint);

} // namespace noctalia::render
