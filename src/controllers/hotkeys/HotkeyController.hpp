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
        std::map<QString, std::function<void(std::vector<QString>)>> actionMap,
        QWidget *parent);

    void save() override;
    std::shared_ptr<Hotkey> getHotkeyByName(QString name);
    void replaceHotkey(QString oldName, std::shared_ptr<Hotkey> newHotkey);
    boost::optional<HotkeyScope> hotkeyScopeFromName(QString scopeName);
    QString hotkeyScopeToName(HotkeyScope scope);

    const std::vector<QString> hotkeyScopeNames = {
        "tab",      "split",    "splitInput", "window",
        "userCard", "settings", "emotePopup", "selectChannelPopup"};

    const std::vector<QString> hotkeyScopeDisplayNames = {
        "Tab",
        "Split",
        "Split input box",
        "Window",
        "User card",
        "Settings dialog",
        "Emote popup",
        "Select channel popup"  //
    };
    pajlada::Signals::NoArgSignal onItemsUpdated;

private:
    SignalVector<std::shared_ptr<Hotkey>> hotkeys_;
    pajlada::Signals::SignalHolder signalHolder_;

    void loadHotkeys();
    void saveHotkeys();
    void resetToDefaults(std::set<QString> &addedHotkeys);
    void tryAddDefault(std::set<QString> &addedHotkeys, HotkeyScope scope,
                       QKeySequence keySequence, QString action,
                       std::vector<QString> args, QString name);
    friend class KeyboardSettingsPage;
};

}  // namespace chatterino
