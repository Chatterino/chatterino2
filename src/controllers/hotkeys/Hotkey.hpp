#pragma once

#include "HotkeyScope.hpp"

#include <QString>

namespace chatterino {

class Hotkey
{
public:
    Hotkey(HotkeyScope scope, QKeySequence keySequence, QString action,
           std::vector<QString> arguments, QString name);
    virtual ~Hotkey() = default;

    /**
     * @brief Returns the OS-specific string representation of the hotkey
     *
     * Suitable for showing in the GUI
     * e.g. Ctrl+F5 or Command+F5
     */
    QString toString() const;

    /**
     * @brief Returns the portable string representation of the hotkey
     *
     * Suitable for saving to/loading from file
     * e.g. Ctrl+F5 or Shift+Ctrl+R
     */
    QString toPortableString() const;

    /**
     * @brief Returns the scope where this hotkey is active
     *
     * See enum HotkeyScope for more information about the various hotkey scopes
     */
    HotkeyScope scope() const;

    /**
     * @brief Returns the action which describes what this Hotkey is meant to do
     *
     * For example, in the Settings scope there's a "search" action which activates the search input box
     */
    QString action() const;

    /**
     * @brief Returns a list of arguments this hotkey has bound to it
     *
     * Some actions require a set of arguments that the user can provide, for example the "openTab" action takes an argument for which tab to switch to. can be a number or a word like next or previous
     */
    std::vector<QString> arguments() const;

    /**
     * @brief Returns the display name of the hotkey
     *
     * For example, in the Settings scope there's a "search" action that has a default hotkey with the name "default search in settings shortcut"
     */
    QString name() const;

    /**
     * @brief Returns the text representation of the hotkeys scope
     *
     * TODO: Should we solidify the wording of Scope and Category?
     */
    QString getCategory() const;

private:
    HotkeyScope scope_;
    QKeySequence keySequence_;
    QString action_;
    std::vector<QString> arguments_;
    QString name_;

    /**
     * @brief Returns the programmating key sequence of the hotkey
     *
     * The actual key codes required for the hotkey to trigger specifically on e.g CTRL+F5
     */
    const QKeySequence &keySequence() const;

    /**
     * @brief Returns the programmatic context of the hotkey to help Qt decide how to apply the hotkey
     *
     * The returned value is based off the hotkeys given scope
     */
    Qt::ShortcutContext getContext() const;

    friend class HotkeyController;
};

}  // namespace chatterino
