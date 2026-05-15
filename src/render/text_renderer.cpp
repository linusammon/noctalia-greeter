#include "render/text_renderer.h"

#include "core/resource_paths.h"

#include <algorithm>
#include <cairo.h>
#include <format>
#include <pango/pangocairo.h>
#include <pango/pangofc-fontmap.h>
#include <stdexcept>
#include <vector>

namespace noctalia::render {

  TextRenderer::~TextRenderer() {
    for (auto& [key, texture] : m_cache) {
      (void)key;
      destroy(texture);
    }
  }

  TextTexture TextRenderer::render(const std::string& text, int fontSize, int maxWidth) {
    return renderWithFont(text, "sans-serif", fontSize, maxWidth);
  }

  TextTexture TextRenderer::renderWithFont(const std::string& text, const char* family, int fontSize, int maxWidth) {
    static const bool fontLoaded = [] {
      FcConfig* config = FcConfigGetCurrent();
      const auto path = core::assetPath("fonts/tabler.ttf").string();
      const bool ok = FcConfigAppFontAddFile(config, reinterpret_cast<const FcChar8*>(path.c_str())) == FcTrue;
      FcConfigBuildFonts(config);
      if (auto* fontMap = PANGO_FC_FONT_MAP(pango_cairo_font_map_get_default()); fontMap != nullptr) {
        pango_fc_font_map_config_changed(fontMap);
      }
      return ok;
    }();
    (void)fontLoaded;

    const int surfaceWidth = std::max(1, maxWidth);
    cairo_surface_t* measureSurface = cairo_image_surface_create(CAIRO_FORMAT_A8, surfaceWidth, 1);
    cairo_t* measure = cairo_create(measureSurface);
    PangoLayout* layout = pango_cairo_create_layout(measure);
    PangoFontDescription* font = pango_font_description_from_string(family);
    pango_font_description_set_absolute_size(font, fontSize * PANGO_SCALE);
    pango_layout_set_font_description(layout, font);
    pango_layout_set_width(layout, surfaceWidth * PANGO_SCALE);
    pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
    pango_layout_set_text(layout, text.c_str(), static_cast<int>(text.size()));

    int pangoWidth = 0;
    int pangoHeight = 0;
    pango_layout_get_pixel_size(layout, &pangoWidth, &pangoHeight);
    const int width = std::max(1, std::min(surfaceWidth, pangoWidth + 2));
    const int height = std::max(1, pangoHeight + 2);

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_A8, width, height);
    cairo_t* cr = cairo_create(surface);
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_move_to(cr, 1.0, 1.0);
    pango_cairo_update_layout(cr, layout);
    pango_cairo_show_layout(cr, layout);
    cairo_surface_flush(surface);

    const int stride = cairo_image_surface_get_stride(surface);
    const unsigned char* data = cairo_image_surface_get_data(surface);
    std::vector<unsigned char> tight(static_cast<std::size_t>(width * height));
    for (int y = 0; y < height; ++y) {
      std::copy_n(data + y * stride, width, tight.data() + y * width);
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tight.data());

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    pango_font_description_free(font);
    g_object_unref(layout);
    cairo_destroy(measure);
    cairo_surface_destroy(measureSurface);
    return {.texture = tex, .width = width, .height = height};
  }

  const TextTexture& TextRenderer::cached(const std::string& text, const char* family, int fontSize, int maxWidth) {
    const std::string key = std::format("{}\n{}\n{}\n{}", family, fontSize, maxWidth, text);
    if (const auto it = m_cache.find(key); it != m_cache.end()) {
      return it->second;
    }
    auto [it, inserted] = m_cache.emplace(key, renderWithFont(text, family, fontSize, maxWidth));
    (void)inserted;
    return it->second;
  }

  void TextRenderer::destroy(TextTexture& texture) {
    if (texture.texture != 0) {
      glDeleteTextures(1, &texture.texture);
      texture.texture = 0;
    }
  }

} // namespace noctalia::render
