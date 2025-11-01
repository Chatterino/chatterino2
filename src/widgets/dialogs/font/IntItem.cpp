#include "widgets/dialogs/font/IntItem.hpp"

namespace chatterino {

IntItem::IntItem(int v, QListWidget *parent)
    : QListWidgetItem(QString::number(v), parent, IntItem::typeId())
    , value(v)
{
}

bool IntItem::operator<(const QListWidgetItem &other) const
{
    const auto *cast = dynamic_cast<const IntItem *>(&other);
    assert(cast);
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

int IntItem::typeId()
{
    return QListWidgetItem::UserType + 123;
}

IntItem *findIntItemInList(QListWidget *list, int value)
{
    for (int n = list->count(), i = 0; i < n; ++i)
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
