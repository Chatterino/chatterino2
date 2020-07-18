#include "widgets/dialogs/switcher/SwitcherItem.hpp"

namespace chatterino {

SwitcherItem::SwitcherItem(const QString &text)
    : QListWidgetItem(text)
{
}

SwitcherItem::SwitcherItem(const QIcon &icon, const QString &text)
    : QListWidgetItem(icon, text)
{
}

}  // namespace chatterino
