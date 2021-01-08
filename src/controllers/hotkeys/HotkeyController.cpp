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
        else
        {
            qCDebug(chatterinoHotkeys) << "Unknown scope: " << scopeName;
            continue;
        }
        this->hotkeys_.append(std::make_shared<Hotkey>(
            scope, QKeySequence(keySequence), action, arguments));
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
