# Custom Hotkeys

## Table of Contents

- [Adding new hotkeys](#Adding_new_hotkeys)
- [Adding new hotkey scopes](#Adding_new_hotkey_scopes)

## Adding new hotkeys

Adding new hotkeys to a widget that already has hotkeys is quite easy.

### Add an action

1.  Locate the call to `getApp()->hotkeys->shortcutsForScope(...)`
2.  Above that should be a map of name to `std::function<QString(std::vector<QString>)>`
3.  Add your new action inside that map, it should return a non-empty QString only when configuration errors are found.

### Add a default

Defaults are stored in `HotkeyController.cpp` in `resetToDefaults`. To add a default just add a call to `tryAddDefault`. Make sure that the name you gave the hotkey is unique.

```
void HotkeyController::tryAddDefault(std::set<QString> &addedHotkeys,
                                     HotkeyScope scope,
                                     QKeySequence keySequence, QString action,
                                     std::vector<QString> args, QString name)
```

- where `action` is the action you added before,
- `scope` — same scope that is in the `shortcutsForScope` call
- `name` — **unique** name of the default hotkey
- `keySequence` - key combo for the hotkey

## Adding new hotkey scopes

If you want to add hotkeys to new widget that doesn't already have them it's a bit more work.

### Add the `HotkeyScope` value

Add a value for the `HotkeyScope` enum in `HotkeyScope.hpp`, please don't reorder the values.

### Add a nice name for the scope

Add a string name and display name for the scope in `HotkeyController.hpp`. Order of these values is important, please don't reorder these.
