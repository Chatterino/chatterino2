#pragma once

namespace chatterino {

enum class HotkeyScope {
    // The HotkeyScope enum describes where the hotkeys's action takes place.
    // Each HotkeyScope represents a widget that has customizable hotkeys. This
    // is needed because more than one widget can have the same or similar action.
    EmotePopup,
    SelectChannelPopup,
    Settings,
    Split,
    SplitInput,
    Tab,
    UserCard,
    Window,
};
}  // namespace chatterino
