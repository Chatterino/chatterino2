#pragma once

#include <QLayout>
#include <boost/variant.hpp>

class QWidget;
class QScrollArea;

namespace chatterino {

using LayoutItem = boost::variant<QWidget *, QLayout *>;

QWidget *wrapLayout(QLayout *layout);
QScrollArea *makeScrollArea(LayoutItem item);

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
                t->addItem(boost::get<QLayout *>(item));
                break;
        }
    }

    t->setContentsMargins(0, 0, 0, 0);

    return t;
}

template <typename T>
T *makeStretchingLayout(std::initializer_list<LayoutItem> items)
{
    auto layout = makeLayout<T>(items);
    layout->addStretch(1);
    return layout;
}

template <typename T, typename With>
T *makeWidget(With with)
{
    auto t = new T;

    with(t);

    return t;
}

}  // namespace chatterino
