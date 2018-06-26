#pragma once

#include "widgets/BaseWidget.hpp"

#include <QPropertyAnimation>
#include <QWidget>

namespace chatterino {

class NotebookPageDropPreview : public BaseWidget
{
public:
    NotebookPageDropPreview(BaseWidget *parent);

    void setBounds(const QRect &rect);

protected:
    void paintEvent(QPaintEvent *) override;
    void hideEvent(QHideEvent *) override;

    //    QPropertyAnimation positionAnimation;
    //    QRect desiredGeometry;
    //    bool animate = false;
};

}  // namespace chatterino
