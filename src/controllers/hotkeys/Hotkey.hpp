#pragma once

#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <QKeySequence>
#include <QString>

#include <vector>

namespace chatterino {

class Hotkey
{
public:
    Hotkey(HotkeyCategory category, QKeySequence keySequence, QString action,
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
     * @brief Returns the category where this hotkey is active. This is labeled the "Category" in the UI.
     *
     * See enum HotkeyCategory for more information about the various hotkey categories
     */
    HotkeyCategory category() const;

    /**
     * @brief Returns the action which describes what this Hotkey is meant to do
     *
     * For example, in the Window category there's a "showSearch" action which opens a search popup
     */
    QString action() const;

    bool validAction() const;

    /**
     * @brief Returns a list of arguments this hotkey has bound to it
     *
     * Some actions require a set of arguments that the user can provide, for example the "openTab" action takes an argument for which tab to switch to. can be a number or a word like next or previous
     */
    std::vector<QString> arguments() const;

    /**
     * @brief Returns the display name of the hotkey
     *
     * For example, in the Split category there's a "showSearch" action that has a default hotkey with the name "default show search"
     */
    QString name() const;

    /**
     * @brief Returns the user-friendly text representation of the hotkeys category
     *
     * Suitable for showing in the GUI.
     * e.g. Split input box for HotkeyCategory::SplitInput
     */
    QString getCategory() const;

    /**
     * @brief Returns the programmating key sequence of the hotkey
     *
     * The actual key codes required for the hotkey to trigger specifically on e.g CTRL+F5
     */
    const QKeySequence &keySequence() const;

private:
    HotkeyCategory category_;
    QKeySequence keySequence_;
    QString action_;
    std::vector<QString> arguments_;
    QString name_;

    /**
     * @brief Returns the programmatic context of the hotkey to help Qt decide how to apply the hotkey
     *
     * The returned value is based off the hotkeys given category
     */
    Qt::ShortcutContext getContext() const;

    friend class HotkeyController;
};

}  // namespace chatterino
