// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/Variant.hpp"

#include <QLayout>

#include <variant>

class QWidget;
class QScrollArea;

namespace chatterino {

using LayoutItem = std::variant<QWidget *, QLayoutItem *>;
using WidgetOrLayout = std::variant<QWidget *, QLayout *>;

QWidget *wrapLayout(QLayout *layout);
QScrollArea *makeScrollArea(WidgetOrLayout item);

template <typename T>
T *makeLayout(std::initializer_list<LayoutItem> items)
{
    auto t = new T;

    for (const auto &item : items)
    {
        std::visit(variant::Overloaded{
                       [&](QWidget *item) {
                           t->addItem(new QWidgetItem(item));
                       },
                       [&](QLayoutItem *item) {
                           t->addItem(item);
                       },
                   },
                   item);
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
