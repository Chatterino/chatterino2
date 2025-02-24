#include "controllers/hotkeys/Hotkey.hpp"

#include "common/QLogging.hpp"
#include "controllers/hotkeys/ActionNames.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"

namespace chatterino {

Hotkey::Hotkey(HotkeyCategory category, QKeySequence keySequence,
               QString action, std::vector<QString> arguments, QString name)
    : category_(category)
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

HotkeyCategory Hotkey::category() const
{
    return this->category_;
}

QString Hotkey::action() const
{
    return this->action_;
}

bool Hotkey::validAction() const
{
    auto categoryActionsIt = actionNames.find(this->category_);
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
    return hotkeyCategoryDisplayName(this->category_);
}

Qt::ShortcutContext Hotkey::getContext() const
{
    switch (this->category_)
    {
        case HotkeyCategory::Window:
            return Qt::WindowShortcut;
        case HotkeyCategory::Split:
            return Qt::WidgetWithChildrenShortcut;
        case HotkeyCategory::SplitInput:
            return Qt::WidgetWithChildrenShortcut;
        case HotkeyCategory::PopupWindow:
            return Qt::WindowShortcut;
    }
    qCDebug(chatterinoHotkeys)
        << "Using default shortcut context for" << this->getCategory()
        << "and hoping for the best.";
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
