#pragma once

#include "render/shader_program.h"

#include <GLES2/gl2.h>
#include <string>
#include <unordered_map>

namespace noctalia::render {

  struct TextTexture {
    GLuint texture = 0;
    int width = 0;
    int height = 0;
  };

  class TextRenderer {
  public:
    ~TextRenderer();

    TextTexture render(const std::string& text, int fontSize, int maxWidth);
    TextTexture renderWithFont(const std::string& text, const char* family, int fontSize, int maxWidth);
    const TextTexture& cached(const std::string& text, const char* family, int fontSize, int maxWidth);
    void destroy(TextTexture& texture);

  private:
    std::unordered_map<std::string, TextTexture> m_cache;
  };

} // namespace noctalia::render
