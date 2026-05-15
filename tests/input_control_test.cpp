#include "ui/controls/input.h"

#include <cassert>
#include <xkbcommon/xkbcommon-keysyms.h>

int main() {
  noctalia::ui::Input input;
  input.insertText("hello");
  input.handleKey('a', 'a', noctalia::ui::KeyModifierCtrl);
  input.insertText("x");
  assert(input.value() == "x");

  input.insertText(" world");
  input.handleKey(XKB_KEY_Left, 0, noctalia::ui::KeyModifierCtrl | noctalia::ui::KeyModifierShift);
  input.handleKey(XKB_KEY_BackSpace, 0, 0);
  assert(input.value() == "x ");

  input.insertText("nótt");
  input.backspace();
  assert(input.value() == "x nót");

  input.handleKey(XKB_KEY_Home, 0, 0);
  input.handleKey(XKB_KEY_Delete, 0, noctalia::ui::KeyModifierCtrl);
  assert(input.value() == " nót");

  return 0;
}
