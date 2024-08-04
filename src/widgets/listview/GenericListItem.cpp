#include "widgets/listview/GenericListItem.hpp"

namespace chatterino {

const QSize GenericListItem::ICON_SIZE(32, 32);

GenericListItem *GenericListItem::fromVariant(const QVariant &variant)
{
    // See https://stackoverflow.com/a/44503822 .
    return static_cast<GenericListItem *>(variant.value<void *>());
}

GenericListItem::GenericListItem()
{
}

GenericListItem::GenericListItem(const QIcon &icon)
    : icon_(icon)
{
}

}  // namespace chatterino
