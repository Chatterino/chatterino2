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
    auto value =
        pajlada::Settings::SettingManager::getInstance()->get("/hotkeys");
    if (value->IsArray())
    {
        auto array = value->GetArray();
        for (auto &hotkeyValue : array)
        {
            qCDebug(chatterinoHotkeys) << "value";
            if (!hotkeyValue.IsObject())
            {
                continue;
            }
            auto hotkeyObject = hotkeyValue.GetObject();
            auto &scopeName = hotkeyObject.FindMember("scope")->value;
            if (!scopeName.IsString())
            {
                qCDebug(chatterinoHotkeys) << "Failed to deserialize hotkey #"
                                           << this->hotkeys_.raw().size() + 1
                                           << ": scope is not a String";
                continue;
            }
            auto &keySequence = hotkeyObject.FindMember("keySequence")->value;
            if (!keySequence.IsString())
            {
                qCDebug(chatterinoHotkeys) << "Failed to deserialize hotkey #"
                                           << this->hotkeys_.raw().size() + 1
                                           << ": keySequence is not a String";
                continue;
            }
            auto &action = hotkeyObject.FindMember("action")->value;
            if (!action.IsString())
            {
                qCDebug(chatterinoHotkeys) << "Failed to deserialize hotkey #"
                                           << this->hotkeys_.raw().size() + 1
                                           << ": action is not a String";
                continue;
            }
            auto &argumentsRaw = hotkeyObject.FindMember("arguments")->value;
            if (!argumentsRaw.IsArray())
            {
                qCDebug(chatterinoHotkeys) << "Failed to deserialize hotkey #"
                                           << this->hotkeys_.raw().size() + 1
                                           << ": arguments is not an Array";
                continue;
            }
            HotkeyScope scope;
            QString scopeNameStr =
                scopeName.GetString();  // XXX: will read until the first \x00.
            if (scopeNameStr == "tab")
            {
                scope = HotkeyScope::Tab;
            }
            else if (scopeNameStr == "split")
            {
                scope = HotkeyScope::Split;
            }
            else if (scopeNameStr == "splitInput")
            {
                scope = HotkeyScope::SplitInput;
            }
            else if (scopeNameStr == "window")
            {
                scope = HotkeyScope::Window;
            }
            else
            {
                qCDebug(chatterinoHotkeys) << "Unknown scope: " << scopeNameStr;
                continue;
            }
            std::vector<QString> arguments;
            if (argumentsRaw.IsArray())
            {
                int argCounter = 0;
                for (auto &val : argumentsRaw.GetArray())
                {
                    if (!val.IsString())
                    {
                        qCDebug(chatterinoHotkeys)
                            << "Invalid type of arugment at argument #"
                            << argCounter
                            << "won't process any arguments after it!";
                        break;
                    }
                    arguments.push_back(val.GetString());
                    argCounter++;
                }
            }

            this->hotkeys_.append(std::make_shared<Hotkey>(
                scope, QKeySequence(QString(keySequence.GetString())),
                QString(action.GetString()), arguments));
        }
    }
    else
    {
        // load defaults here
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
