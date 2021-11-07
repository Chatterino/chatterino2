#include "controllers/hotkeys/Hotkey.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/ActionNames.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"

namespace chatterino {

Hotkey::Hotkey(HotkeyScope scope, QKeySequence keySequence, QString action,
               std::vector<QString> arguments, QString name)
    : scope_(scope)
    , keySequence_(keySequence)
    , action_(action)
    , arguments_(arguments)
    , name_(name)
{
}

const QKeySequence &Hotkey::keySequence() const
{
    return this->keySequence_;
}

QString Hotkey::name() const
{
    return this->name_;
}

HotkeyScope Hotkey::scope() const
{
    return this->scope_;
}

QString Hotkey::action() const
{
    return this->action_;
}

bool Hotkey::validAction() const
{
    auto categoryActionsIt = actionNames.find(this->scope_);
    if (categoryActionsIt == actionNames.end())
    {
        // invalid category
        return false;
    }

    auto actionDefinitionIt = categoryActionsIt->second.find(this->action());

    return actionDefinitionIt != categoryActionsIt->second.end();
}

std::vector<QString> Hotkey::arguments() const
{
    return this->arguments_;
}

QString Hotkey::getCategory() const
{
    return getApp()
        ->hotkeys->hotkeyScopeDisplayNames.find(this->scope_)
        ->second;
}

Qt::ShortcutContext Hotkey::getContext() const
{
    switch (this->scope_)
    {
        case HotkeyScope::Window:
            return Qt::WindowShortcut;
        case HotkeyScope::Split:
            return Qt::WidgetWithChildrenShortcut;
        case HotkeyScope::SplitInput:
            return Qt::WidgetWithChildrenShortcut;
        case HotkeyScope::PopupWindow:
            return Qt::WindowShortcut;
    }
    qCDebug(chatterinoHotkeys)
        << "Using default shortcut context for" << this->getCategory()
        << "and hopeing for the best.";
    return Qt::WidgetShortcut;
}

QString Hotkey::toString() const
{
    return this->keySequence().toString(QKeySequence::NativeText);
}

QString Hotkey::toPortableString() const
{
    return this->keySequence().toString(QKeySequence::PortableText);
}

}  // namespace chatterino
