#include "widgets/dialogs/font/IntItem.hpp"

namespace chatterino {

IntItem::IntItem(int v, QListWidget *parent)
    : QListWidgetItem(QString::number(v), parent, IntItem::TYPE_ID)
    , value(v)
{
}

bool IntItem::operator<(const QListWidgetItem &other) const
{
    const auto *cast = dynamic_cast<const IntItem *>(&other);
    if (cast == nullptr)
    {
        assert(cast && "IntItem may only be compared with other list items");
        return false;
    }

    return this->value < cast->value;
}

void IntItem::setValue(int v)
{
    this->value = v;
    QListWidgetItem::setText(QString::number(v));
}

int IntItem::getValue() const
{
    return this->value;
}

IntItem *findIntItemInList(QListWidget *list, int value)
{
    int numItems = list->count();

    for (int i = 0; i < numItems; ++i)
    {
        auto *item = dynamic_cast<IntItem *>(list->item(i));
        if (item && item->getValue() == value)
        {
            return item;
        }
    }

    return nullptr;
}

}  // namespace chatterino
