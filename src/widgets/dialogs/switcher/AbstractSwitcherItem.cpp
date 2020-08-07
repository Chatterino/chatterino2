#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

#include "Application.hpp"

namespace chatterino {

const QSize AbstractSwitcherItem::ICON_SIZE(32, 32);

AbstractSwitcherItem *AbstractSwitcherItem::fromVariant(const QVariant &variant)
{
    // See https://stackoverflow.com/a/44503822 .
    return static_cast<AbstractSwitcherItem *>(variant.value<void *>());
}

AbstractSwitcherItem::AbstractSwitcherItem(const QIcon &icon)
    : icon_(icon)
{
}

}  // namespace chatterino
