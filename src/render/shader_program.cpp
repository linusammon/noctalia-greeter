#include "render/shader_program.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace noctalia::render {
  namespace {
    GLuint compileShader(GLenum type, const char* source) {
      const GLuint shader = glCreateShader(type);
      if (shader == 0) {
        throw std::runtime_error("glCreateShader failed");
      }
      glShaderSource(shader, 1, &source, nullptr);
      glCompileShader(shader);
      GLint ok = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
      if (ok == GL_FALSE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error("shader compilation failed: " + log);
      }
      return shader;
    }
  } // namespace

  ShaderProgram::~ShaderProgram() { destroy(); }

  void ShaderProgram::create(const char* vertexSource, const char* fragmentSource) {
    destroy();
    const GLuint vertex = compileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragment = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    m_program = glCreateProgram();
    glAttachShader(m_program, vertex);
    glAttachShader(m_program, fragment);
    glLinkProgram(m_program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    GLint ok = 0;
    glGetProgramiv(m_program, GL_LINK_STATUS, &ok);
    if (ok == GL_FALSE) {
      GLint length = 0;
      glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &length);
      std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
      glGetProgramInfoLog(m_program, length, nullptr, log.data());
      destroy();
      throw std::runtime_error("shader link failed: " + log);
    }
  }

  void ShaderProgram::destroy() {
    if (m_program != 0) {
      glDeleteProgram(m_program);
      m_program = 0;
    }
  }

} // namespace noctalia::render
