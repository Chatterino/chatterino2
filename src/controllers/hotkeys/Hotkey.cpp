#include "Hotkey.hpp"

#include "common/QLogging.hpp"

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

std::vector<QString> Hotkey::arguments() const
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
        case HotkeyScope::UserCard:
            return "User card";
        case HotkeyScope::Settings:
            return "Settings dialog";
        case HotkeyScope::EmotePopup:
            return "Emote popup";
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
        case HotkeyScope::UserCard:
            return Qt::WindowShortcut;
        case HotkeyScope::Settings:
            return Qt::WidgetShortcut;
        case HotkeyScope::EmotePopup:
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
