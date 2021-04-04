#pragma once
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "controllers/hotkeys/HotkeyScope.hpp"

namespace chatterino {
class Hotkey;
class Settings;
class Paths;

class HotkeyModel;

class HotkeyController final : public Singleton
{
public:
    HotkeyController();
    HotkeyModel *createModel(QObject *parent);

    virtual void initialize(Settings &settings, Paths &paths) override;
    std::vector<QShortcut *> shortcutsForScope(
        HotkeyScope scope,
        std::map<QString, std::function<QString(std::vector<QString>)>>
            actionMap,
        QWidget *parent);

    void save() override;
    std::shared_ptr<Hotkey> getHotkeyByName(QString name);
    void replaceHotkey(QString oldName, std::shared_ptr<Hotkey> newHotkey);
    boost::optional<HotkeyScope> hotkeyScopeFromName(QString scopeName);
    QString hotkeyScopeToName(HotkeyScope scope);

    const std::vector<QString> hotkeyScopeNames = {
        "emotePopup", "selectChannelPopup", "settings", "split", "splitInput",
        "tab",        "userCard",           "window"};

    const std::vector<QString> hotkeyScopeDisplayNames = {
        "Emote popup",  //
        "Select channel popup",
        "Settings dialog",
        "Split",
        "Split input box",
        "Tab",
        "User card",
        "Window",
    };
    pajlada::Signals::NoArgSignal onItemsUpdated;
    std::map<HotkeyScope, std::set<QString>> savedActions;

private:
    SignalVector<std::shared_ptr<Hotkey>> hotkeys_;
    pajlada::Signals::SignalHolder signalHolder_;

    void loadHotkeys();
    void saveHotkeys();
    void resetToDefaults(std::set<QString> &addedHotkeys);
    void tryAddDefault(std::set<QString> &addedHotkeys, HotkeyScope scope,
                       QKeySequence keySequence, QString action,
                       std::vector<QString> args, QString name);
    void showHotkeyError(std::shared_ptr<Hotkey> hotkey, QString warning);
    friend class KeyboardSettingsPage;
};

}  // namespace chatterino
