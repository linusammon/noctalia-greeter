#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace noctalia::ui {

  enum KeyModifier : std::uint32_t {
    KeyModifierShift = 1u << 0u,
    KeyModifierCtrl = 1u << 1u,
    KeyModifierAlt = 1u << 2u,
    KeyModifierSuper = 1u << 3u,
  };

  class Input {
  public:
    void setPlaceholder(std::string placeholder);
    void setPasswordMode(bool enabled) noexcept;
    void setFocused(bool focused) noexcept;
    void setValue(std::string value);

    void insertText(std::string_view text);
    void backspace();
    void deleteForward();
    void clear();
    bool handleKey(std::uint32_t sym, std::uint32_t utf32, std::uint32_t modifiers);
    void selectAll();
    void clearSelection();
    void moveCaretLeft(bool shift = false, bool word = false);
    void moveCaretRight(bool shift = false, bool word = false);
    void moveCaretHome(bool shift = false);
    void moveCaretEnd(bool shift = false);

    [[nodiscard]] const std::string& value() const noexcept { return m_value; }
    [[nodiscard]] const std::string& placeholder() const noexcept { return m_placeholder; }
    [[nodiscard]] bool passwordMode() const noexcept { return m_passwordMode; }
    [[nodiscard]] bool focused() const noexcept { return m_focused; }
    [[nodiscard]] bool empty() const noexcept { return m_value.empty(); }
    [[nodiscard]] std::string displayValue() const;
    [[nodiscard]] std::size_t glyphCount() const noexcept;
    [[nodiscard]] std::size_t cursorGlyphIndex() const noexcept;
    [[nodiscard]] std::size_t selectionStartGlyphIndex() const noexcept;
    [[nodiscard]] std::size_t selectionEndGlyphIndex() const noexcept;
    [[nodiscard]] bool hasSelection() const noexcept;

  private:
    [[nodiscard]] std::size_t selectionStart() const noexcept;
    [[nodiscard]] std::size_t selectionEnd() const noexcept;
    void deleteSelection();
    [[nodiscard]] std::size_t glyphIndexForByte(std::size_t byteOffset) const noexcept;
    [[nodiscard]] std::size_t previousWordStartForByteOffset(std::size_t offset) const;
    [[nodiscard]] std::size_t nextWordStartForByteOffset(std::size_t offset) const;
    [[nodiscard]] std::size_t nextWordEndForByteOffset(std::size_t offset) const;

    static bool isWordCodepoint(const std::string& text, std::size_t byteOffset);
    static std::size_t nextCharPos(const std::string& text, std::size_t byteOffset);
    static std::size_t prevCharPos(const std::string& text, std::size_t byteOffset);
    static std::string utf32ToUtf8(std::uint32_t codepoint);

    std::string m_value;
    std::string m_placeholder;
    std::size_t m_cursorPos = 0;
    std::size_t m_selectionAnchor = 0;
    bool m_passwordMode = false;
    bool m_focused = false;
  };

} // namespace noctalia::ui
