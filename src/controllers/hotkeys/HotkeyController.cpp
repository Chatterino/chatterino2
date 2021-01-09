#include "HotkeyController.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyModel.hpp"
#include "singletons/Settings.hpp"

#include <QShortcut>

namespace chatterino {
HotkeyController::HotkeyController()
{
    this->loadHotkeys();
}

void HotkeyController::initialize(Settings &settings, Paths &paths)
{
}

void HotkeyController::loadHotkeys()
{
    auto keys = pajlada::Settings::SettingManager::getObjectKeys("/hotkeys");
    if (keys.size() == 0)
    {
        this->resetToDefaults();
        return;
    }

    qCDebug(chatterinoHotkeys) << "Loading hotkeys...";
    for (const auto &key : keys)
    {
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
        HotkeyScope scope;
        if (scopeName == "tab")
        {
            scope = HotkeyScope::Tab;
        }
        else if (scopeName == "split")
        {
            scope = HotkeyScope::Split;
        }
        else if (scopeName == "splitInput")
        {
            scope = HotkeyScope::SplitInput;
        }
        else if (scopeName == "window")
        {
            scope = HotkeyScope::Window;
        }
        else if (scopeName == "userCard")
        {
            scope = HotkeyScope::UserCard;
        }
        else
        {
            qCDebug(chatterinoHotkeys) << "Unknown scope: " << scopeName;
            continue;
        }
        this->hotkeys_.append(
            std::make_shared<Hotkey>(scope, QKeySequence(keySequence), action,
                                     arguments, QString::fromStdString(key)));
    }
}

void HotkeyController::resetToDefaults()
{
    // split
    {
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::Split, QKeySequence("Ctrl+W"), "delete",
            std::vector<QString>(), "default delete shortcut"));
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::Split, QKeySequence("Ctrl+R"), "changeChannel",
            std::vector<QString>(), "default change channel shortcut"));
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::Split, QKeySequence("Ctrl+F"), "showSearch",
            std::vector<QString>(), "default show search shortcut"));
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::Split, QKeySequence("Ctrl+F5"), "reconnect",
            std::vector<QString>(), "default reconnect shortcut"));
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::Split, QKeySequence("F10"), "debug",
            std::vector<QString>(), "default debug shortcut"));

        {
            std::vector<QString> args;
            args.push_back("left");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+h"), "focus", args,
                "default vim focus left shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("down");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+j"), "focus", args,
                "default vim focus down shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("up");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+k"), "focus", args,
                "default vim focus up shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("right");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+l"), "focus", args,
                "default vim focus right shortcut"));
        }

        {
            std::vector<QString> args;
            args.push_back("left");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+left"), "focus", args,
                "default focus left shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("down");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+down"), "focus", args,
                "default focus down shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("up");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+up"), "focus", args,
                "default focus up shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("right");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("Alt+right"), "focus", args,
                "default focus right shortcut"));
        }

        {
            std::vector<QString> args;
            args.push_back("up");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("PageUp"), "scrollPage", args,
                "default page up shortcut"));
        }
        {
            std::vector<QString> args;
            args.push_back("down");
            this->hotkeys_.append(std::make_shared<Hotkey>(
                HotkeyScope::Split, QKeySequence("PageDown"), "scrollPage",
                args, "default page down shortcut"));
        }
    }

    // split input
    {
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::SplitInput, QKeySequence("Ctrl+E"), "openEmotesPopup",
            std::vector<QString>(), "default emote picker shortcut"));
    }

    // user card
    {
        this->hotkeys_.append(std::make_shared<Hotkey>(
            HotkeyScope::UserCard, QKeySequence("Escape"), "delete",
            std::vector<QString>(), "default close user card shortcut"));
    }
}

void HotkeyController::save()
{
    this->saveHotkeys();
}

void HotkeyController::saveHotkeys()
{
    for (auto &hotkey : this->hotkeys_)
    {
        auto section = "/hotkeys/" + hotkey->name().toStdString();
        pajlada::Settings::Setting<QString>::set(section + "/action",
                                                 hotkey->action());
        pajlada::Settings::Setting<QString>::set(
            section + "/keySequence", hotkey->keySequence().toString());
        QString scopeName;
        switch (hotkey->scope())
        {
            case HotkeyScope::Tab:
                scopeName = "tab";
                break;
            case HotkeyScope::Split:
                scopeName = "split";
                break;
            case HotkeyScope::SplitInput:
                scopeName = "splitInput";
                break;
            case HotkeyScope::Window:
                scopeName = "window";
                break;
            case HotkeyScope::UserCard:
                scopeName = "userCard";
                break;
        }

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
    qCDebug(chatterinoHotkeys) << "Registering hotkeys...";
    std::vector<QShortcut *> output;
    for (const auto &hotkey : this->hotkeys_)
    {
        qCDebug(chatterinoHotkeys)
            << "FDM " << hotkey->getCategory() << hotkey->action();
        if (hotkey->scope() != scope)
        {
            qCDebug(chatterinoHotkeys) << "Skipping: bad scope";
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

}  // namespace chatterino
