#pragma once

#include <QLayout>
#include <boost/variant.hpp>

class QWidget;
class QScrollArea;

namespace chatterino {

using LayoutItem = boost::variant<QWidget *, QLayoutItem *>;
using WidgetOrLayout = boost::variant<QWidget *, QLayout *>;

QWidget *wrapLayout(QLayout *layout);
QScrollArea *makeScrollArea(WidgetOrLayout item);

template <typename T>
T *makeLayout(std::initializer_list<LayoutItem> items)
{
    auto t = new T;

    for (auto &item : items)
    {
        switch (item.which())
        {
            case 0:
                t->addItem(new QWidgetItem(boost::get<QWidget *>(item)));
                break;
            case 1:
                t->addItem(boost::get<QLayoutItem *>(item));
                break;
        }
    }

    t->setContentsMargins(0, 0, 0, 0);

    return t;
}

template <typename T, typename With>
T *makeWidget(With with)
{
    auto t = new T;

    with(t);

    return t;
}

}  // namespace chatterino
