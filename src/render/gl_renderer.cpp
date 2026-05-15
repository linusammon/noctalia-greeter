#include "render/gl_renderer.h"

#include "core/resource_paths.h"
#include "render/glyph_registry.h"

#include <GLES2/gl2.h>
#include <algorithm>
#include <array>
#include <cairo.h>
#include <librsvg/rsvg.h>
#include <stdexcept>
#include <vector>

namespace noctalia::render {
  namespace {
    constexpr char kTextVertexShader[] = R"(
precision highp float;
attribute vec2 a_position;
uniform vec2 u_surface_size;
uniform vec4 u_rect;
varying vec2 v_texcoord;
vec2 to_ndc(vec2 pixel_pos) {
    vec2 normalized = pixel_pos / u_surface_size;
    return vec2(normalized.x * 2.0 - 1.0, 1.0 - normalized.y * 2.0);
}
void main() {
    v_texcoord = a_position;
    gl_Position = vec4(to_ndc(u_rect.xy + a_position * u_rect.zw), 0.0, 1.0);
}
)";

    constexpr char kTextFragmentShader[] = R"(
precision mediump float;
uniform vec4 u_color;
uniform sampler2D u_texture;
varying vec2 v_texcoord;
void main() {
    float alpha = texture2D(u_texture, v_texcoord).a;
    gl_FragColor = vec4(u_color.rgb * u_color.a * alpha, u_color.a * alpha);
}
)";

    constexpr char kImageFragmentShader[] = R"(
precision mediump float;
uniform vec4 u_color;
uniform sampler2D u_texture;
varying vec2 v_texcoord;
void main() {
    vec4 tex = texture2D(u_texture, v_texcoord);
    gl_FragColor = tex * u_color;
}
)";
  } // namespace

  GlRenderer::~GlRenderer() {
    if (m_logo.texture != 0) {
      glDeleteTextures(1, &m_logo.texture);
    }
    if (m_quadBuffer != 0) {
      glDeleteBuffers(1, &m_quadBuffer);
    }
  }

  void GlRenderer::initialize() {
    m_rectProgram.ensureInitialized();
    m_textProgram.create(kTextVertexShader, kTextFragmentShader);
    m_imageProgram.create(kTextVertexShader, kImageFragmentShader);
    const std::array<float, 8> quad{0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f};
    glGenBuffers(1, &m_quadBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(quad.size() * sizeof(float)), quad.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  void GlRenderer::beginFrame(int width, int height) {
    m_width = width > 0 ? width : 1;
    m_height = height > 0 ? height : 1;
    glViewport(0, 0, m_width, m_height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_SCISSOR_TEST);
    glClearColor(0.035f, 0.036f, 0.045f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  void GlRenderer::drawRect(float x, float y, float width, float height, float radius, Color color, Color border,
                            float borderWidth) {
    RoundedRectStyle style{
        .fill = ::Color{.r = color.r, .g = color.g, .b = color.b, .a = color.a},
        .border = ::Color{.r = border.r, .g = border.g, .b = border.b, .a = border.a},
        .fillMode = FillMode::Solid,
        .radius = radius,
        .softness = 1.0f,
        .borderWidth = borderWidth,
    };
    m_rectProgram.draw(static_cast<float>(m_width), static_cast<float>(m_height), width, height, style,
                       Mat3::translation(x, y));
  }

  void GlRenderer::drawTextBlocks(float x, float y, const std::vector<std::string>& lines, Color color) {
    std::string text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
      if (i > 0) {
        text += '\n';
      }
      text += lines[i];
    }
    drawText(x, y, text, static_cast<float>(m_width) - x - 32.0f, 15, color);
  }

  void GlRenderer::drawText(float x, float y, const std::string& text, float maxWidth, int fontSize, Color color) {
    const TextTexture& texture = m_textRenderer.cached(text, "sans-serif", fontSize, static_cast<int>(maxWidth));
    drawTexture(m_textProgram.id(), texture.texture, x, y, static_cast<float>(texture.width),
                static_cast<float>(texture.height), color);
  }

  void GlRenderer::drawGlyph(float x, float y, std::string_view name, int fontSize, Color color) {
    const TextTexture& texture =
        m_textRenderer.cached(utf8(glyphCodepoint(name)), "noctalia-tabler-icons", fontSize, fontSize + 8);
    drawTexture(m_textProgram.id(), texture.texture, x, y, static_cast<float>(texture.width),
                static_cast<float>(texture.height), color);
  }

  void GlRenderer::drawNoctaliaLogo(float x, float y, float size) {
    const int textureSize = static_cast<int>(std::max(1.0f, size));
    if (m_logo.texture == 0 || m_logo.width != textureSize) {
      if (m_logo.texture != 0) {
        glDeleteTextures(1, &m_logo.texture);
      }
      m_logo = loadSvgTexture("noctalia.svg", textureSize);
    }
    if (m_logo.texture != 0) {
      drawTexture(m_imageProgram.id(), m_logo.texture, x, y, size, size, Color{1.0f, 1.0f, 1.0f, 1.0f});
    }
  }

  void GlRenderer::drawTexture(GLuint program, GLuint texture, float x, float y, float width, float height,
                               Color color) {
    glUseProgram(program);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadBuffer);
    const GLint pos = glGetAttribLocation(program, "a_position");
    glEnableVertexAttribArray(static_cast<GLuint>(pos));
    glVertexAttribPointer(static_cast<GLuint>(pos), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glUniform2f(glGetUniformLocation(program, "u_surface_size"), static_cast<float>(m_width),
                static_cast<float>(m_height));
    glUniform4f(glGetUniformLocation(program, "u_rect"), x, y, width, height);
    glUniform4f(glGetUniformLocation(program, "u_color"), color.r, color.g, color.b, color.a);
    glUniform1i(glGetUniformLocation(program, "u_texture"), 0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(static_cast<GLuint>(pos));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  GlRenderer::ImageTexture GlRenderer::loadSvgTexture(const char* assetName, int size) {
    GError* error = nullptr;
    const auto path = core::assetPath(assetName).string();
    RsvgHandle* handle = rsvg_handle_new_from_file(path.c_str(), &error);
    if (handle == nullptr) {
      if (error != nullptr) {
        g_error_free(error);
      }
      return {};
    }

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t* cr = cairo_create(surface);
    RsvgRectangle viewport{.x = 0.0, .y = 0.0, .width = static_cast<double>(size), .height = static_cast<double>(size)};
    if (!rsvg_handle_render_document(handle, cr, &viewport, &error)) {
      if (error != nullptr) {
        g_error_free(error);
      }
      cairo_destroy(cr);
      cairo_surface_destroy(surface);
      g_object_unref(handle);
      return {};
    }
    cairo_surface_flush(surface);

    const int stride = cairo_image_surface_get_stride(surface);
    const auto* source = cairo_image_surface_get_data(surface);
    std::vector<unsigned char> rgba(static_cast<std::size_t>(size * size * 4));
    for (int y = 0; y < size; ++y) {
      for (int x = 0; x < size; ++x) {
        const unsigned char* pixel = source + y * stride + x * 4;
        const std::size_t out = static_cast<std::size_t>((y * size + x) * 4);
        rgba[out + 0] = pixel[2];
        rgba[out + 1] = pixel[1];
        rgba[out + 2] = pixel[0];
        rgba[out + 3] = pixel[3];
      }
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    g_object_unref(handle);
    return {.texture = texture, .width = size, .height = size};
  }

} // namespace noctalia::render
