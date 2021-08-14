#pragma once

namespace chatterino {

// HotkeyScope describes where the hotkeys's action takes place.
// Each HotkeyScope represents a widget that has customizable hotkeys. This
// is needed because more than one widget can have the same or similar action.
enum class HotkeyScope {
    PopupWindow,
    Split,
    SplitInput,
    Window,
};

}  // namespace chatterino
