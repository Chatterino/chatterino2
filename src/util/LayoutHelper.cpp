// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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

    std::visit(variant::Overloaded{
                   [&](QWidget *item) {
                       area->setWidget(item);
                   },
                   [&](QLayout *item) {
                       area->setWidget(wrapLayout(item));
                   },
               },
               item);

    area->setWidgetResizable(true);

    return area;
}

}  // namespace chatterino
