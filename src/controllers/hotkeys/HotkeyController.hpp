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
     * @returns the new index in the SignalVector
     **/
    int replaceHotkey(QString oldName, std::shared_ptr<Hotkey> newHotkey);
    boost::optional<HotkeyCategory> hotkeyCategoryFromName(
        QString categoryName);
    QString hotkeyCategoryToName(HotkeyCategory category);
    bool isDuplicate(std::shared_ptr<Hotkey> hotkey, QString ignoreNamed);

    const std::map<HotkeyCategory, QString> hotkeyCategoryNames = {
        {HotkeyCategory::PopupWindow, "popupWindow"},
        {HotkeyCategory::Split, "split"},
        {HotkeyCategory::SplitInput, "splitInput"},
        {HotkeyCategory::Window, "window"}};

    const std::map<HotkeyCategory, QString> hotkeyCategoryDisplayNames = {
        {HotkeyCategory::PopupWindow, "Popup Windows"},
        {HotkeyCategory::Split, "Split"},
        {HotkeyCategory::SplitInput, "Split input box"},
        {HotkeyCategory::Window, "Window"},
    };
    pajlada::Signals::NoArgSignal onItemsUpdated;

private:
    SignalVector<std::shared_ptr<Hotkey>> hotkeys_;
    pajlada::Signals::SignalHolder signalHolder_;

    void loadHotkeys();
    void saveHotkeys();
    void addDefaults(std::set<QString> &addedHotkeys);
    void resetToDefaults();
    void tryAddDefault(std::set<QString> &addedHotkeys, HotkeyCategory category,
                       QKeySequence keySequence, QString action,
                       std::vector<QString> args, QString name);
    void showHotkeyError(std::shared_ptr<Hotkey> hotkey, QString warning);
    friend class KeyboardSettingsPage;
};

}  // namespace chatterino
