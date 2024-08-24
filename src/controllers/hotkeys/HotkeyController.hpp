#pragma once

#include "common/SignalVector.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <optional>
#include <set>

class QShortcut;

namespace chatterino {

class Hotkey;

class HotkeyModel;

/**
 * @returns a const map with the HotkeyCategory enum as its key, and HotkeyCategoryData as the value.
 **/
[[nodiscard]] const std::map<HotkeyCategory, HotkeyCategoryData> &
    hotkeyCategories();

/**
 * @brief Returns the name of the given hotkey category
 *
 * @returns the name, or an empty string if an invalid hotkey category was given
 **/
[[nodiscard]] QString hotkeyCategoryName(HotkeyCategory category);

/**
 * @brief Returns the display name of the given hotkey category
 *
 * @returns the display name, or an empty string if an invalid hotkey category was given
 **/
[[nodiscard]] QString hotkeyCategoryDisplayName(HotkeyCategory category);

class HotkeyController final
{
public:
    using HotkeyFunction = std::function<QString(std::vector<QString>)>;
    using HotkeyMap = std::map<QString, HotkeyFunction>;

    HotkeyController();
    HotkeyModel *createModel(QObject *parent);

    std::vector<QShortcut *> shortcutsForCategory(HotkeyCategory category,
                                                  HotkeyMap actionMap,
                                                  QWidget *parent);

    void save();
    std::shared_ptr<Hotkey> getHotkeyByName(QString name);
    /**
     * @brief returns a QKeySequence that perfoms the actions requested.
     * Accepted if and only if the category matches, the action matches and arguments match.
     * When arguments is present, contents of arguments must match the checked hotkey, otherwise arguments are ignored.
     * For example:
     * - std::nullopt (or {}) will match any hotkey satisfying category, action values,
     * - {{"foo", "bar"}} will only match a hotkey that has these arguments and these arguments only
     */
    QKeySequence getDisplaySequence(
        HotkeyCategory category, const QString &action,
        const std::optional<std::vector<QString>> &arguments = {}) const;

    /**
     * @brief removes the hotkey with the oldName and inserts newHotkey at the end
     *
     * @returns the new index in the SignalVector
     **/
    int replaceHotkey(QString oldName, std::shared_ptr<Hotkey> newHotkey);
    std::optional<HotkeyCategory> hotkeyCategoryFromName(QString categoryName);

    /**
     * @brief checks if the hotkey is duplicate
     *
     * @param hotkey the hotkey to check
     * @param ignoreNamed name of hotkey to ignore. Useful for ensuring we don't fail if the hotkey's name is being edited
     *
     * @returns true if the given hotkey is a duplicate, false if it's not
     **/
    [[nodiscard]] bool isDuplicate(std::shared_ptr<Hotkey> hotkey,
                                   QString ignoreNamed);

    pajlada::Signals::NoArgSignal onItemsUpdated;

    /**
     * @brief Removes hotkeys that were previously added as default hotkeys.
     *
     * This will potentially remove hotkeys that were explicitly added by the user if they added a hotkey
     * with the exact same parameters as the default hotkey.
     */
    void clearRemovedDefaults();

    /// Returns the names of removed or deprecated hotkeys the user had at launch, if any
    ///
    /// This is used to populate the on-launch warning in the hotkey dialog
    const std::set<QString> &removedOrDeprecatedHotkeys() const;

private:
    /**
     * @brief load hotkeys from under the /hotkeys settings path
     **/
    void loadHotkeys();

    /**
     * @brief save hotkeys to the /hotkeys path
     *
     * This is done by first fully clearing the /hotkeys object, then reapplying all hotkeys
     * from the hotkeys_ object
     **/
    void saveHotkeys();

    /**
     * @brief try to load all default hotkeys
     *
     * New hotkeys must be added to this function
     **/
    void addDefaults(std::set<QString> &addedHotkeys);

    /**
     * @brief remove all user-made changes to hotkeys and reset to the default hotkeys
     **/
    void resetToDefaults();

    /**
     * @brief try to add a hotkey if it hasn't already been added or modified by the user
     **/
    void tryAddDefault(std::set<QString> &addedHotkeys, HotkeyCategory category,
                       QKeySequence keySequence, QString action,
                       std::vector<QString> args, QString name);

    /**
     * @brief try to remove a default hotkey if it hasn't already been modified by the user
     *
     * NOTE: This could also remove a user-added hotkey assuming it matches all parameters
     *
     * @returns true if the hotkey was removed
     **/
    bool tryRemoveDefault(HotkeyCategory category, QKeySequence keySequence,
                          QString action, std::vector<QString> args,
                          QString name);

    /// Add hotkeys matching the given arguments to list of removed/deprecated hotkeys
    /// that the user should remove
    void warnForRemovedHotkeyActions(HotkeyCategory category, QString action,
                                     std::vector<QString> args);

    /**
     * @brief show an error dialog about a hotkey in a standard format
     **/
    static void showHotkeyError(const std::shared_ptr<Hotkey> &hotkey,
                                QString warning);
    /**
     * @brief finds a Hotkey matching category, action and arguments.
     * Accepted if and only if the category matches, the action matches and arguments match.
     * When arguments is present, contents of arguments must match the checked hotkey, otherwise arguments are ignored.
     * For example:
     * - std::nullopt (or {}) will match any hotkey satisfying category, action values,
     * - {{"foo", "bar"}} will only match a hotkey that has these arguments and these arguments only
     */
    std::shared_ptr<Hotkey> findLike(
        HotkeyCategory category, const QString &action,
        const std::optional<std::vector<QString>> &arguments = {}) const;

    friend class KeyboardSettingsPage;

    /// Stores a list of names the user had at launch that contained deprecated or removed hotkey actions
    std::set<QString> removedOrDeprecatedHotkeys_;

    SignalVector<std::shared_ptr<Hotkey>> hotkeys_;
    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
