#pragma once

#include "widgets/basewidget.hpp"

#include <QPropertyAnimation>
#include <QWidget>

namespace chatterino {
namespace widgets {

class NotebookPageDropPreview : public BaseWidget
{
public:
    NotebookPageDropPreview(BaseWidget *parent);

    void setBounds(const QRect &rect);

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void hideEvent(QHideEvent *) override;

    QPropertyAnimation positionAnimation;
    QRect desiredGeometry;
    bool animate = false;
};

}  // namespace widgets
}  // namespace chatterino
