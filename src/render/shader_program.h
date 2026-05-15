#pragma once

#include <GLES2/gl2.h>

namespace noctalia::render {

  class ShaderProgram {
  public:
    ~ShaderProgram();
    ShaderProgram() = default;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    void create(const char* vertexSource, const char* fragmentSource);
    void destroy();
    GLuint id() const { return m_program; }

  private:
    GLuint m_program = 0;
  };

} // namespace noctalia::render
