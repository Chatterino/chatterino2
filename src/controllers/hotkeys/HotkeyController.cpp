#include "HotkeyController.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "singletons/Settings.hpp"

#include <QShortcut>

namespace chatterino {

boost::optional<HotkeyScope> HotkeyController::hotkeyScopeFromName(
    QString scopeName)
{
    HotkeyScope scope;
    int index = 0;
    for (auto ref : this->hotkeyScopeNames)
    {
        if (ref == scopeName)
        {
            return HotkeyScope(index);
        }
        index++;
    }
    qCDebug(chatterinoHotkeys) << "Unknown scope: " << scopeName;
    return {};
}

QString HotkeyController::hotkeyScopeToName(HotkeyScope scope)
{
    unsigned long scopeId = (unsigned long)(scope);
    return 0 <= scopeId && scopeId <= this->hotkeyScopeNames.size()
               ? this->hotkeyScopeNames.at(scopeId)
               : "";
}

HotkeyController::HotkeyController()
{
    this->loadHotkeys();
    this->signalHolder_.managedConnect(
        this->hotkeys_.delayedItemsChanged, [this]() {
            qCDebug(chatterinoHotkeys) << "Reloading hotkeys!";
            this->onItemsUpdated.invoke();
        });
}

void HotkeyController::initialize(Settings &settings, Paths &paths)
{
}

void HotkeyController::loadHotkeys()
{
    auto defaultHotkeysAdded =
        pajlada::Settings::Setting<std::vector<QString>>::get(
            "/hotkeys/addedDefaults");
    auto set = std::set<QString>(defaultHotkeysAdded.begin(),
                                 defaultHotkeysAdded.end());

    auto keys = pajlada::Settings::SettingManager::getObjectKeys("/hotkeys");
    this->resetToDefaults(set);
    pajlada::Settings::Setting<std::vector<QString>>::set(
        "/hotkeys/addedDefaults", std::vector<QString>(set.begin(), set.end()));

    qCDebug(chatterinoHotkeys) << "Loading hotkeys...";
    for (const auto &key : keys)
    {
        if (key == "addedDefaults")
        {
            continue;
        }

        auto section = "/hotkeys/" + key;
        auto scopeName =
            pajlada::Settings::Setting<QString>::get(section + "/scope");
        auto keySequence =
            pajlada::Settings::Setting<QString>::get(section + "/keySequence");
        auto action =
            pajlada::Settings::Setting<QString>::get(section + "/action");
        auto arguments = pajlada::Settings::Setting<std::vector<QString>>::get(
            section + "/arguments");
        qCDebug(chatterinoHotkeys)
            << "Hotkey " << scopeName << keySequence << action << arguments;

        if (scopeName.isEmpty() || keySequence.isEmpty() || action.isEmpty())
        {
            continue;
        }
        auto scope = HotkeyController::hotkeyScopeFromName(scopeName);
        if (!scope)
        {
            continue;
        }
        this->hotkeys_.append(
            std::make_shared<Hotkey>(*scope, QKeySequence(keySequence), action,
                                     arguments, QString::fromStdString(key)));
    }
}

void HotkeyController::tryAddDefault(std::set<QString> &addedHotkeys,
                                     HotkeyScope scope,
                                     QKeySequence keySequence, QString action,
                                     std::vector<QString> args, QString name)
{
    qCDebug(chatterinoHotkeys) << "Try add default" << name;
    if (addedHotkeys.count(name) != 0)
    {
        qCDebug(chatterinoHotkeys) << "Already exists";
        return;  // hotkey was added before
    }
    qCDebug(chatterinoHotkeys) << "Inserted";
    this->hotkeys_.append(
        std::make_shared<Hotkey>(scope, keySequence, action, args, name));
    addedHotkeys.insert(name);
}

void HotkeyController::resetToDefaults(std::set<QString> &addedHotkeys)
{
    // split
    {
        this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                            QKeySequence("Ctrl+W"), "delete",
                            std::vector<QString>(), "default delete shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                            QKeySequence("Ctrl+R"), "changeChannel",
                            std::vector<QString>(),
                            "default change channel shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                            QKeySequence("Ctrl+F"), "showSearch",
                            std::vector<QString>(),
                            "default show search shortcut");
        this->tryAddDefault(
            addedHotkeys, HotkeyScope::Split, QKeySequence("Ctrl+F5"),
            "reconnect", std::vector<QString>(), "default reconnect shortcut");

        this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                            QKeySequence("F10"), "debug",
                            std::vector<QString>(), "default debug shortcut");

        this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                            QKeySequence("Alt+x"), "createClip",
                            std::vector<QString>(),
                            "default create clip shortcut");

        {
            std::vector<QString> args;
            args.push_back("left");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+h"), "focus", args,
                                "default vim focus left shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("down");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+j"), "focus", args,
                                "default vim focus down shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("up");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+k"), "focus", args,
                                "default vim focus up shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("right");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+l"), "focus", args,
                                "default vim focus right shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("left");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+left"), "focus", args,
                                "default focus left shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("down");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+down"), "focus", args,
                                "default focus down shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("up");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+up"), "focus", args,
                                "default focus up shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("right");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("Alt+right"), "focus", args,
                                "default focus right shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("up");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("PgUp"), "scrollPage", args,
                                "default scroll page up shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("down");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                                QKeySequence("PgDown"), "scrollPage", args,
                                "default scroll page down shortcut");
        }
        this->tryAddDefault(addedHotkeys, HotkeyScope::Split,
                            QKeySequence("Ctrl+End"), "scrollToBottom",
                            std::vector<QString>(),
                            "default scroll to bottom shortcut");
    }

    // split input
    {
        this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                            QKeySequence("Ctrl+E"), "openEmotesPopup",
                            std::vector<QString>(),
                            "default emote picker shortcut");

        // all variations of send message :)
        {
            this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                                QKeySequence("Enter"), "sendMessage",
                                std::vector<QString>(),
                                "default send message shortcut");
            {
                std::vector<QString> args;
                args.push_back("keepInput");
                this->tryAddDefault(
                    addedHotkeys, HotkeyScope::SplitInput,
                    QKeySequence("Ctrl+Enter"), "sendMessage", args,
                    "default send message and keep text shortcut");
            }
            this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                                QKeySequence("Return"), "sendMessage",
                                std::vector<QString>(),
                                "default send message shortcut 2");
            {
                std::vector<QString> args;
                args.push_back("keepInput");
                this->tryAddDefault(
                    addedHotkeys, HotkeyScope::SplitInput,
                    QKeySequence("Ctrl+Return"), "sendMessage", args,
                    "default send message and keep text shortcut 2");
            }

            this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                                QKeySequence("Shift+Enter"), "sendMessage",
                                std::vector<QString>(),
                                "default send message shortcut 3");
            {
                std::vector<QString> args;
                args.push_back("keepInput");
                this->tryAddDefault(
                    addedHotkeys, HotkeyScope::SplitInput,
                    QKeySequence("Ctrl+Shift+Enter"), "sendMessage", args,
                    "default send message and keep text shortcut 3");
            }
            this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                                QKeySequence("Shift+Return"), "sendMessage",
                                std::vector<QString>(),
                                "default send message shortcut 4");
            {
                std::vector<QString> args;
                args.push_back("keepInput");
                this->tryAddDefault(
                    addedHotkeys, HotkeyScope::SplitInput,
                    QKeySequence("Ctrl+Shift+Return"), "sendMessage", args,
                    "default send message and keep text shortcut 4");
            }
        }

        {
            std::vector<QString> args;
            args.push_back("start");
            args.push_back("withoutSelection");
            this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                                QKeySequence("Home"), "jumpCursor", args,
                                "default go to start of input shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("end");
            args.push_back("withoutSelection");
            this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                                QKeySequence("End"), "jumpCursor", args,
                                "default go to end of input shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("start");
            args.push_back("withSelection");
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::SplitInput,
                QKeySequence("Shift+Home"), "jumpCursor", args,
                "default go to start of input with selection shortcut");
        }
        {
            std::vector<QString> args;
            args.push_back("end");
            args.push_back("withSelection");
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::SplitInput,
                QKeySequence("Shift+End"), "jumpCursor", args,
                "default go to end of input with selection shortcut");
        }

        this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                            QKeySequence("Up"), "previousMessage",
                            std::vector<QString>(),
                            "default previous message shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::SplitInput,
                            QKeySequence("Down"), "nextMessage",
                            std::vector<QString>(),
                            "default next message shortcut");
    }

    // user card
    {
        this->tryAddDefault(addedHotkeys, HotkeyScope::UserCard,
                            QKeySequence("Escape"), "delete",
                            std::vector<QString>(),
                            "default close user card shortcut");
    }

    // window
    {
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("Ctrl+P"), "openSettings",
                            std::vector<QString>(),
                            "default open settings shortcut");
        this->tryAddDefault(
            addedHotkeys, HotkeyScope::Window, QKeySequence("Ctrl+T"),
            "newSplit", std::vector<QString>(), "default new split shortcut");
        for (int i = 0; i < 9; i++)
        {
            std::vector<QString> args;
            args.push_back(QString::number(i));
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::Window,
                QKeySequence(QString("Ctrl+%1").arg(i + 1)), "openTab", args,
                QString("default select tab #%1 shortcut").arg(i + 1));
        }
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("Ctrl+9"), "openTab",
                            std::vector<QString>(),
                            "default select last tab shortcut");

        {
            std::vector<QString> args;
            args.push_back("next");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                                QKeySequence("Ctrl+Tab"), "openTab", args,
                                "default select next tab shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("previous");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                                QKeySequence("Ctrl+Shift+Tab"), "openTab", args,
                                "default select previous tab shortcut");
        }

        this->tryAddDefault(
            addedHotkeys, HotkeyScope::Window, QKeySequence("Ctrl+N"), "popup",
            std::vector<QString>(), "default new popup window shortcut");

        {
            std::vector<QString> args;

            args.push_back("in");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                                QKeySequence::ZoomIn, "zoom", args,
                                "default zoom in shortcut");
        }

        {
            std::vector<QString> args;

            args.push_back("out");
            this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                                QKeySequence::ZoomOut, "zoom", args,
                                "default zoom out shortcut");
        }

        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("Ctrl+Shift+T"), "newTab",
                            std::vector<QString>(), "default new tab shortcut");

        this->tryAddDefault(
            addedHotkeys, HotkeyScope::Window, QKeySequence("Ctrl+Shift+W"),
            "removeTab", std::vector<QString>(), "default remove tab shortcut");

        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("Ctrl+G"), "reopenSplit",
                            std::vector<QString>(),
                            "default reopen split shortcut");

        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("Ctrl+H"), "toggleLocalR9K",
                            std::vector<QString>(),
                            "default toggle local r9k shortcut");

        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("Ctrl+K"), "openQuickSwitcher",
                            std::vector<QString>(),
                            "default open quick switcher shortcut");
#ifdef C_DEBUG
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("F6"), "addMiscMessage",
                            std::vector<QString>(),
                            "default debug add misc message shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("F7"), "addCheerMessage",
                            std::vector<QString>(),
                            "default debug add cheer message shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("F8"), "addLinkMessage",
                            std::vector<QString>(),
                            "default debug add link message shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("F9"), "addRewardMessage",
                            std::vector<QString>(),
                            "default debug add reward message shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::Window,
                            QKeySequence("F10"), "addEmoteMessage",
                            std::vector<QString>(),
                            "default debug add emote message shortcut");
#endif
    }

    // settings
    {
        this->tryAddDefault(addedHotkeys, HotkeyScope::Settings,
                            QKeySequence("Ctrl+F"), "search",
                            std::vector<QString>(),
                            "default search in settings shortcut");
    }

    // emote popup
    {
        for (int i = 0; i < 9; i++)
        {
            std::vector<QString> args;
            args.push_back(QString::number(i));
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::EmotePopup,
                QKeySequence(QString("Ctrl+%1").arg(i + 1)), "openTab", args,
                QString("default emote popup select tab #%1 shortcut")
                    .arg(i + 1));
        }
        {
            std::vector<QString> args;
            args.push_back("last");
            this->tryAddDefault(addedHotkeys, HotkeyScope::EmotePopup,
                                QKeySequence("Ctrl+9"), "openTab",
                                std::vector<QString>(),
                                "default emote popup select last tab shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("next");
            this->tryAddDefault(addedHotkeys, HotkeyScope::EmotePopup,
                                QKeySequence("Ctrl+Tab"), "openTab", args,
                                "default emote popup select next tab shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("previous");
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::EmotePopup,
                QKeySequence("Ctrl+Shift+Tab"), "openTab", args,
                "default emote popup select previous tab shortcut");
        }
    }

    // select channel popup
    {
        for (int i = 0; i < 9; i++)
        {
            std::vector<QString> args;
            args.push_back(QString::number(i));
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::SelectChannelPopup,
                QKeySequence(QString("Ctrl+%1").arg(i + 1)), "openTab", args,
                QString("default select channel popup select tab #%1 shortcut")
                    .arg(i + 1));
        }
        {
            std::vector<QString> args;
            args.push_back("last");
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::EmotePopup, QKeySequence("Ctrl+9"),
                "openTab", std::vector<QString>(),
                "default select channel popup select last tab shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("next");
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::SelectChannelPopup,
                QKeySequence("Ctrl+Tab"), "openTab", args,
                "default select channel popup select next tab shortcut");
        }

        {
            std::vector<QString> args;
            args.push_back("previous");
            this->tryAddDefault(
                addedHotkeys, HotkeyScope::SelectChannelPopup,
                QKeySequence("Ctrl+Shift+Tab"), "openTab", args,
                "default select channel popup select previous tab shortcut");
        }
        this->tryAddDefault(addedHotkeys, HotkeyScope::SelectChannelPopup,
                            QKeySequence("Return"), "accept",
                            std::vector<QString>(),
                            "default select channel popup accept shortcut");
        this->tryAddDefault(addedHotkeys, HotkeyScope::SelectChannelPopup,
                            QKeySequence("Escape"), "reject",
                            std::vector<QString>(),
                            "default select channel popup reject shortcut");
    }
}

void HotkeyController::save()
{
    this->saveHotkeys();
}

void HotkeyController::saveHotkeys()
{
    auto defaultHotkeysAdded =
        pajlada::Settings::Setting<std::vector<QString>>::get(
            "/hotkeys/addedDefaults");

    // make sure that hotkeys are deleted
    pajlada::Settings::SettingManager::getInstance()->set(
        "/hotkeys", rapidjson::Value(rapidjson::kObjectType));

    // re-add /hotkeys/addedDefaults as previous set call deleted that key
    pajlada::Settings::Setting<std::vector<QString>>::set(
        "/hotkeys/addedDefaults",
        std::vector<QString>(defaultHotkeysAdded.begin(),
                             defaultHotkeysAdded.end()));

    for (auto &hotkey : this->hotkeys_)
    {
        auto section = "/hotkeys/" + hotkey->name().toStdString();
        pajlada::Settings::Setting<QString>::set(section + "/action",
                                                 hotkey->action());
        pajlada::Settings::Setting<QString>::set(
            section + "/keySequence", hotkey->keySequence().toString());

        auto scopeName = HotkeyController::hotkeyScopeToName(hotkey->scope());
        pajlada::Settings::Setting<QString>::set(section + "/scope", scopeName);
        pajlada::Settings::Setting<std::vector<QString>>::set(
            section + "/arguments", hotkey->arguments());
    }
}

HotkeyModel *HotkeyController::createModel(QObject *parent)
{
    HotkeyModel *model = new HotkeyModel(parent);
    model->initialize(&this->hotkeys_);
    return model;
}

std::vector<QShortcut *> HotkeyController::shortcutsForScope(
    HotkeyScope scope,
    std::map<QString, std::function<void(std::vector<QString>)>> actionMap,
    QWidget *parent)
{
    if (this->savedActions.find(scope) == this->savedActions.end())
    {
        qCDebug(chatterinoHotkeys) << "Add saved actions for" << (int)scope;
        std::set<QString> actions;
        for (const auto &pair : actionMap)
        {
            actions.insert(pair.first);
        }
        this->savedActions.insert({scope, actions});
    }
    std::vector<QShortcut *> output;
    for (const auto &hotkey : this->hotkeys_)
    {
        if (hotkey->scope() != scope)
        {
            continue;
        }
        auto target = actionMap.find(hotkey->action());
        if (target == actionMap.end())
        {
            qCDebug(chatterinoHotkeys)
                << "Unknown hotkey action: " << hotkey->action() << "in scope "
                << hotkey->getCategory();
            continue;
        }
        auto s = new QShortcut(QKeySequence(hotkey->keySequence()), parent);
        s->setContext(hotkey->getContext());
        auto functionPointer = target->second;
        QObject::connect(s, &QShortcut::activated, parent,
                         [functionPointer, hotkey]() {
                             qCDebug(chatterinoHotkeys)
                                 << "Shortcut pressed: " << hotkey->action();
                             functionPointer(hotkey->arguments());
                         });
        output.push_back(s);
    }
    return output;
}

std::shared_ptr<Hotkey> HotkeyController::getHotkeyByName(QString name)
{
    for (auto &hotkey : this->hotkeys_)
    {
        if (hotkey->name() == name)
        {
            return hotkey;
        }
    }
    return nullptr;
}

void HotkeyController::replaceHotkey(QString oldName,
                                     std::shared_ptr<Hotkey> newHotkey)
{
    int i = 0;
    for (auto &hotkey : this->hotkeys_)
    {
        if (hotkey->name() == oldName)
        {
            this->hotkeys_.removeAt(i);
            break;
        }
        i++;
    }
    this->hotkeys_.append(newHotkey);
}

}  // namespace chatterino
