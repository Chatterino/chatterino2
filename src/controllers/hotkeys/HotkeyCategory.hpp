#pragma once

namespace chatterino {

// HotkeyCategory describes where the hotkeys's action takes place.
// Each HotkeyCategory represents a widget that has customizable hotkeys. This
// is needed because more than one widget can have the same or similar action.
enum class HotkeyCategory {
    PopupWindow,
    Split,
    SplitInput,
    Window,
};

}  // namespace chatterino
