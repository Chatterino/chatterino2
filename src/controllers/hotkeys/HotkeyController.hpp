#pragma once

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <boost/optional.hpp>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <set>

class QShortcut;

namespace chatterino {

class Hotkey;

class HotkeyModel;

class HotkeyController final : public Singleton
{
public:
    using HotkeyFunction = std::function<QString(std::vector<QString>)>;
    using HotkeyMap = std::map<QString, HotkeyFunction>;

    HotkeyController();
    HotkeyModel *createModel(QObject *parent);

    std::vector<QShortcut *> shortcutsForCategory(HotkeyCategory category,
                                                  HotkeyMap actionMap,
                                                  QWidget *parent);

    void save() override;
    std::shared_ptr<Hotkey> getHotkeyByName(QString name);

    /**
     * @brief removes the hotkey with the oldName and inserts newHotkey at the end
     *
     * @returns the new index in the SignalVector
     **/
    int replaceHotkey(QString oldName, std::shared_ptr<Hotkey> newHotkey);
    boost::optional<HotkeyCategory> hotkeyCategoryFromName(
        QString categoryName);

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

    /**
     * @brief Returns the display name of the given hotkey category
     *
     * @returns the display name, or an empty string if an invalid hotkey category was given
     **/
    [[nodiscard]] QString categoryDisplayName(HotkeyCategory category) const;

    /**
     * @brief Returns the name of the given hotkey category
     *
     * @returns the name, or an empty string if an invalid hotkey category was given
     **/
    [[nodiscard]] QString categoryName(HotkeyCategory category) const;

    /**
     * @returns a const map with the HotkeyCategory enum as its key, and HotkeyCategoryData as the value.
     **/
    [[nodiscard]] const std::map<HotkeyCategory, HotkeyCategoryData>
        &categories() const;

    pajlada::Signals::NoArgSignal onItemsUpdated;

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
     * @brief show an error dialog about a hotkey in a standard format
     **/
    static void showHotkeyError(const std::shared_ptr<Hotkey> &hotkey,
                                QString warning);

    friend class KeyboardSettingsPage;

    SignalVector<std::shared_ptr<Hotkey>> hotkeys_;
    pajlada::Signals::SignalHolder signalHolder_;

    const std::map<HotkeyCategory, HotkeyCategoryData> hotkeyCategories_ = {
        {HotkeyCategory::PopupWindow, {"popupWindow", "Popup Windows"}},
        {HotkeyCategory::Split, {"split", "Split"}},
        {HotkeyCategory::SplitInput, {"splitInput", "Split input box"}},
        {HotkeyCategory::Window, {"window", "Window"}},
    };
};

}  // namespace chatterino
