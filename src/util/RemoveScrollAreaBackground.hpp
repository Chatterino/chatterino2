#pragma once

#include <QScrollArea>

namespace chatterino {

static void removeScrollAreaBackground(QScrollArea *scrollArea,
                                       QWidget *childWidget)
{
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(0);

    QPalette p;
    p.setColor(QPalette::Background, QColor(0, 0, 0, 0));
    scrollArea->setPalette(p);
    childWidget->setPalette(p);
}

}  // namespace chatterino
