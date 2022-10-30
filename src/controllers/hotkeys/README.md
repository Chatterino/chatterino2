# Custom Hotkeys

## Table of Contents

- [Glossary](#Glossary)
- [Adding new hotkeys](#Adding_new_hotkeys)
- [Adding new hotkey categories](#Adding_new_hotkey_categories)

## Glossary

| Word                    | Meaning                                                                                   |
| ----------------------- | ----------------------------------------------------------------------------------------- |
| Shortcut                | `QShortcut` object created from a hotkey.                                                 |
| Hotkey                  | Template for creating shortcuts in the right categories. See [Hotkey object][hotkey.hpp]. |
| Category                | Place where hotkeys' actions are executed.                                                |
| Action                  | Code that makes a hotkey do something.                                                    |
| Keybinding or key combo | The keys you press on the keyboard to do something.                                       |

## Adding new hotkeys

Adding new hotkeys to a widget that already has hotkeys is quite easy.

### Add an action

1.  Locate the call to `getApp()->hotkeys->shortcutsForCategory(...)`, it is located in the `addShortcuts()` method
2.  Above that should be a `HotkeyController::HotkeyMap` named `actions`
3.  Add your new action inside that map, it should return a non-empty QString only when configuration errors are found.
4.  Go to `ActionNames.hpp` and add a definition for your hotkey with a nice user-friendly name. Be sure to double-check the argument count.

### Add a default

Defaults are stored in `HotkeyController.cpp` in the `resetToDefaults()` method. To add a default just add a call to `tryAddDefault` in the appropriate section. Make sure that the name you gave the hotkey is unique.

```cpp
void HotkeyController::tryAddDefault(std::set<QString> &addedHotkeys,
                                     HotkeyCategory category,
                                     QKeySequence keySequence, QString action,
                                     std::vector<QString> args, QString name)
```

- where `action` is the action you added before,
- `category` — same category that is in the `shortcutsForCategory` call
- `name` — **unique** name of the default hotkey
- `keySequence` - key combo for the hotkey

## Adding new hotkey categories

If you want to add hotkeys to new widget that doesn't already have them it's a bit more work.

### Add the `HotkeyCategory` value

Add a value for the `HotkeyCategory` enum in [`HotkeyCategory.hpp`][hotkeycategory.hpp]. If you widget is a popup, it's best to use the existing `PopupWindow` category.

### Add a nice name for the category

Add a string name and display name for the category in [`HotkeyController.hpp`][hotkeycontroller.hpp] to `hotkeyCategoryNames` and `hotkeyCategoryDisplayNames`.

### Add a shortcut context

To make sure shortcuts created from your hotkeys are only executed in the right places, you need to add a shortcut context for Qt. This is done in `Hotkey.cpp` in `Hotkey::getContext()`.
See the [ShortcutContext enum docs for possible values](https://doc.qt.io/qt-5/qt.html#ShortcutContext-enum)

### Override `addShortcuts`

If the widget you're adding Hotkeys is a `BaseWidget` or a `BaseWindow`. You can override the `addShortcuts()` method. You should also add a call to it in the constructor. Here is some template/example code:

```cpp
void YourWidget::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"barrelRoll", // replace this with your action code
         [this](std::vector<QString> arguments) -> QString {
             // DO A BARREL ROLL
             return ""; // only return text if there is a configuration error.
         }},
    };
    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(HotkeyCategory::PopupWindow /* or your category name */,
                                                            actions, this);
}
```

## Renaming defaults

Renaming defaults is currently not possible. If you were to rename one, it would get recreated for everyone probably leading to broken shortcuts, don't do this until a proper mechanism has been made.

<!-- big list of links -->

[actionnames.hpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/ActionNames.hpp
[hotkey.cpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/Hotkey.cpp
[hotkey.hpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/Hotkey.hpp
[hotkeycontroller.cpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/HotkeyController.cpp
[hotkeycontroller.hpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/HotkeyController.hpp
[hotkeymodel.cpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/HotkeyModel.cpp
[hotkeymodel.hpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/HotkeyModel.hpp
[hotkeycategory.hpp]: https://github.com/Chatterino/chatterino2/blob/custom_hotkeys/src/controllers/hotkeys/HotkeyCategory.hpp
