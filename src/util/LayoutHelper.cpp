#include "util/LayoutHelper.hpp"

#include <QScrollArea>
#include <QWidget>

namespace chatterino {

QWidget *wrapLayout(QLayout *layout)
{
    auto *widget = new QWidget;
    widget->setLayout(layout);
    return widget;
}

QScrollArea *makeScrollArea(WidgetOrLayout item)
{
    auto *area = new QScrollArea();

    switch (item.which())
    {
        case 0:
            area->setWidget(boost::get<QWidget *>(item));
            break;
        case 1:
            area->setWidget(wrapLayout(boost::get<QLayout *>(item)));
            break;
    }

    area->setWidgetResizable(true);

    return area;
}

}  // namespace chatterino
