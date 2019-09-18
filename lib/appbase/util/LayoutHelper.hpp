#pragma once

#include <QLayout>
#include <QWidget>
#include <boost/variant.hpp>

namespace chatterino {

using LayoutItem = boost::variant<QWidget *, QLayout *>;

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
