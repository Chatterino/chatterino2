#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

AbstractSwitcherItem::AbstractSwitcherItem(const QString &text)
    : QListWidgetItem(text)
{
}

AbstractSwitcherItem::AbstractSwitcherItem(const QIcon &icon,
                                           const QString &text)
    : QListWidgetItem(icon, text)
{
}

}  // namespace chatterino
