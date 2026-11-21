// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QScrollArea>

namespace chatterino {

static void removeScrollAreaBackground(QScrollArea *scrollArea,
                                       QWidget *childWidget)
{
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(0);

    QPalette p;
    p.setColor(QPalette::Window, QColor(0, 0, 0, 0));
    scrollArea->setPalette(p);
    childWidget->setPalette(p);
}

}  // namespace chatterino
