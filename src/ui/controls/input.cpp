#include "ui/controls/input.h"

#include <algorithm>
#include <cctype>
#include <utility>
#include <xkbcommon/xkbcommon-keysyms.h>

namespace noctalia::ui {

  void Input::setPlaceholder(std::string placeholder) { m_placeholder = std::move(placeholder); }

  void Input::setPasswordMode(bool enabled) noexcept { m_passwordMode = enabled; }

  void Input::setFocused(bool focused) noexcept { m_focused = focused; }

  void Input::setValue(std::string value) {
    m_value = std::move(value);
    m_cursorPos = m_value.size();
    m_selectionAnchor = m_cursorPos;
  }

  void Input::insertText(std::string_view text) {
    if (text.empty()) {
      return;
    }
    if (hasSelection()) {
      deleteSelection();
    }
    m_value.insert(m_cursorPos, text);
    m_cursorPos += text.size();
    m_selectionAnchor = m_cursorPos;
  }

  void Input::backspace() {
    if (hasSelection()) {
      deleteSelection();
      return;
    }
    if (m_cursorPos == 0) {
      return;
    }
    const std::size_t previous = prevCharPos(m_value, m_cursorPos);
    m_value.erase(previous, m_cursorPos - previous);
    m_cursorPos = previous;
    m_selectionAnchor = m_cursorPos;
  }

  void Input::deleteForward() {
    if (hasSelection()) {
      deleteSelection();
      return;
    }
    if (m_cursorPos >= m_value.size()) {
      return;
    }
    const std::size_t next = nextCharPos(m_value, m_cursorPos);
    m_value.erase(m_cursorPos, next - m_cursorPos);
    m_selectionAnchor = m_cursorPos;
  }

  void Input::clear() {
    m_value.clear();
    m_cursorPos = 0;
    m_selectionAnchor = 0;
  }

  bool Input::handleKey(std::uint32_t sym, std::uint32_t utf32, std::uint32_t modifiers) {
    const bool shift = (modifiers & KeyModifierShift) != 0;
    const bool ctrl = (modifiers & KeyModifierCtrl) != 0;

    if (ctrl && (sym == 'a' || sym == 'A')) {
      selectAll();
      return true;
    }
    if (sym == XKB_KEY_BackSpace) {
      if (ctrl && !hasSelection()) {
        const std::size_t previous = previousWordStartForByteOffset(m_cursorPos);
        m_value.erase(previous, m_cursorPos - previous);
        m_cursorPos = previous;
        m_selectionAnchor = m_cursorPos;
      } else {
        backspace();
      }
      return true;
    }
    if (sym == XKB_KEY_Delete) {
      if (ctrl && !hasSelection()) {
        const std::size_t next = nextWordEndForByteOffset(m_cursorPos);
        m_value.erase(m_cursorPos, next - m_cursorPos);
      } else {
        deleteForward();
      }
      return true;
    }
    if (sym == XKB_KEY_Left) {
      moveCaretLeft(shift, ctrl);
      return true;
    }
    if (sym == XKB_KEY_Right) {
      moveCaretRight(shift, ctrl);
      return true;
    }
    if (sym == XKB_KEY_Home) {
      moveCaretHome(shift);
      return true;
    }
    if (sym == XKB_KEY_End) {
      moveCaretEnd(shift);
      return true;
    }
    if (ctrl || utf32 < 0x20U || utf32 == 0x7FU) {
      return false;
    }

    insertText(utf32ToUtf8(utf32));
    return true;
  }

  void Input::selectAll() {
    m_selectionAnchor = 0;
    m_cursorPos = m_value.size();
  }

  void Input::clearSelection() { m_selectionAnchor = m_cursorPos; }

  void Input::moveCaretLeft(bool shift, bool word) {
    if (!shift && hasSelection()) {
      m_cursorPos = selectionStart();
    } else {
      m_cursorPos = word ? previousWordStartForByteOffset(m_cursorPos) : prevCharPos(m_value, m_cursorPos);
    }
    if (!shift) {
      m_selectionAnchor = m_cursorPos;
    }
  }

  void Input::moveCaretRight(bool shift, bool word) {
    if (!shift && hasSelection()) {
      m_cursorPos = selectionEnd();
    } else {
      m_cursorPos = word ? nextWordStartForByteOffset(m_cursorPos) : nextCharPos(m_value, m_cursorPos);
    }
    if (!shift) {
      m_selectionAnchor = m_cursorPos;
    }
  }

  void Input::moveCaretHome(bool shift) {
    m_cursorPos = 0;
    if (!shift) {
      m_selectionAnchor = m_cursorPos;
    }
  }

  void Input::moveCaretEnd(bool shift) {
    m_cursorPos = m_value.size();
    if (!shift) {
      m_selectionAnchor = m_cursorPos;
    }
  }

  std::string Input::displayValue() const {
    if (!m_passwordMode) {
      return m_value;
    }
    const auto glyphCount =
        std::count_if(m_value.begin(), m_value.end(), [](unsigned char byte) { return (byte & 0xC0) != 0x80; });
    return std::string(static_cast<std::size_t>(glyphCount), '*');
  }

  std::size_t Input::glyphCount() const noexcept { return glyphIndexForByte(m_value.size()); }

  std::size_t Input::cursorGlyphIndex() const noexcept { return glyphIndexForByte(m_cursorPos); }

  std::size_t Input::selectionStartGlyphIndex() const noexcept { return glyphIndexForByte(selectionStart()); }

  std::size_t Input::selectionEndGlyphIndex() const noexcept { return glyphIndexForByte(selectionEnd()); }

  bool Input::hasSelection() const noexcept { return m_cursorPos != m_selectionAnchor; }

  std::size_t Input::selectionStart() const noexcept { return std::min(m_cursorPos, m_selectionAnchor); }

  std::size_t Input::selectionEnd() const noexcept { return std::max(m_cursorPos, m_selectionAnchor); }

  void Input::deleteSelection() {
    const std::size_t start = selectionStart();
    const std::size_t end = selectionEnd();
    m_value.erase(start, end - start);
    m_cursorPos = start;
    m_selectionAnchor = start;
  }

  std::size_t Input::glyphIndexForByte(std::size_t byteOffset) const noexcept {
    byteOffset = std::min(byteOffset, m_value.size());
    return static_cast<std::size_t>(std::count_if(m_value.begin(),
                                                  m_value.begin() + static_cast<std::ptrdiff_t>(byteOffset),
                                                  [](unsigned char byte) { return (byte & 0xC0) != 0x80; }));
  }

  std::size_t Input::previousWordStartForByteOffset(std::size_t offset) const {
    std::size_t pos = std::min(offset, m_value.size());
    while (pos > 0) {
      const std::size_t previous = prevCharPos(m_value, pos);
      if (previous == pos || isWordCodepoint(m_value, previous)) {
        break;
      }
      pos = previous;
    }
    while (pos > 0) {
      const std::size_t previous = prevCharPos(m_value, pos);
      if (previous == pos || !isWordCodepoint(m_value, previous)) {
        break;
      }
      pos = previous;
    }
    return pos;
  }

  std::size_t Input::nextWordStartForByteOffset(std::size_t offset) const {
    std::size_t pos = std::min(offset, m_value.size());
    if (pos < m_value.size() && isWordCodepoint(m_value, pos)) {
      while (pos < m_value.size() && isWordCodepoint(m_value, pos)) {
        pos = nextCharPos(m_value, pos);
      }
    }
    while (pos < m_value.size() && !isWordCodepoint(m_value, pos)) {
      pos = nextCharPos(m_value, pos);
    }
    return pos;
  }

  std::size_t Input::nextWordEndForByteOffset(std::size_t offset) const {
    std::size_t pos = std::min(offset, m_value.size());
    while (pos < m_value.size() && !isWordCodepoint(m_value, pos)) {
      pos = nextCharPos(m_value, pos);
    }
    while (pos < m_value.size() && isWordCodepoint(m_value, pos)) {
      pos = nextCharPos(m_value, pos);
    }
    return pos;
  }

  bool Input::isWordCodepoint(const std::string& text, std::size_t byteOffset) {
    if (byteOffset >= text.size()) {
      return false;
    }
    const auto lead = static_cast<unsigned char>(text[byteOffset]);
    if ((lead & 0x80U) != 0U) {
      return true;
    }
    return std::isalnum(lead) != 0 || lead == '_';
  }

  std::size_t Input::nextCharPos(const std::string& text, std::size_t byteOffset) {
    if (byteOffset >= text.size()) {
      return byteOffset;
    }
    ++byteOffset;
    while (byteOffset < text.size() && (static_cast<unsigned char>(text[byteOffset]) & 0xC0U) == 0x80U) {
      ++byteOffset;
    }
    return byteOffset;
  }

  std::size_t Input::prevCharPos(const std::string& text, std::size_t byteOffset) {
    if (byteOffset == 0) {
      return 0;
    }
    --byteOffset;
    while (byteOffset > 0 && (static_cast<unsigned char>(text[byteOffset]) & 0xC0U) == 0x80U) {
      --byteOffset;
    }
    return byteOffset;
  }

  std::string Input::utf32ToUtf8(std::uint32_t codepoint) {
    std::string result;
    if (codepoint < 0x80U) {
      result += static_cast<char>(codepoint);
    } else if (codepoint < 0x800U) {
      result += static_cast<char>(0xC0U | (codepoint >> 6U));
      result += static_cast<char>(0x80U | (codepoint & 0x3FU));
    } else if (codepoint < 0x10000U) {
      result += static_cast<char>(0xE0U | (codepoint >> 12U));
      result += static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
      result += static_cast<char>(0x80U | (codepoint & 0x3FU));
    } else {
      result += static_cast<char>(0xF0U | (codepoint >> 18U));
      result += static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU));
      result += static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU));
      result += static_cast<char>(0x80U | (codepoint & 0x3FU));
    }
    return result;
  }

} // namespace noctalia::ui
