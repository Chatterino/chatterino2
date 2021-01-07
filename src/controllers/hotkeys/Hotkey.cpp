#include "Hotkey.hpp"

#include "common/QLogging.hpp"

namespace chatterino {

Hotkey::Hotkey(HotkeyScope scope, QKeySequence keySequence, QString action)
    : scope_(scope)
    , keySequence_(keySequence)
    , action_(action)
{
}

const QKeySequence &Hotkey::keySequence() const
{
    return this->keySequence_;
}

HotkeyScope Hotkey::scope() const
{
    return this->scope_;
}

QString Hotkey::action() const
{
    return this->action_;
}

QStringList Hotkey::arguments() const
{
    return this->arguments_;
}

QString Hotkey::getCategory() const
{
    switch (this->scope_)
    {
        case HotkeyScope::Tab:
            return "Tab";
        case HotkeyScope::Window:
            return "Window";
        case HotkeyScope::SplitInput:
            return "Split input box";
        case HotkeyScope::Split:
            return "Split";
        default:
            return "Unknown hotkey scope";
    }
}

Qt::ShortcutContext Hotkey::getContext() const
{
    switch (this->scope_)
    {
        case HotkeyScope::Tab:
            return Qt::WidgetWithChildrenShortcut;
        case HotkeyScope::Window:
            return Qt::WindowShortcut;
        case HotkeyScope::Split:
            return Qt::WidgetWithChildrenShortcut;
        case HotkeyScope::SplitInput:
            return Qt::WidgetShortcut;
    }
    qCDebug(chatterinoHotkeys)
        << "Using default shortcut context for" << this->getCategory()
        << "and hopeing for the best.";
    return Qt::WidgetShortcut;
}

QString Hotkey::toString() const
{
    return this->getCategory();
}

}  // namespace chatterino
