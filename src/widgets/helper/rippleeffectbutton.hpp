#pragma once

#include <boost/optional.hpp>

#include "widgets/basewidget.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QWidget>

namespace chatterino {
namespace widgets {

class RippleEffectButton : public BaseWidget
{
    Q_OBJECT

    struct ClickEffect {
        double progress = 0.0;
        QPoint position;

        ClickEffect(QPoint _position)
            : position(_position)
        {
        }
    };

public:
    RippleEffectButton(BaseWidget *parent);

    void setMouseEffectColor(boost::optional<QColor> color);
    void setPixmap(const QPixmap *pixmap);
    const QPixmap *getPixmap() const;

signals:
    void clicked();

protected:
    bool selected = false;
    bool mouseOver = false;
    bool mouseDown = false;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    void fancyPaint(QPainter &painter);

private:
    QPixmap *pixmap;
    QPoint mousePos;
    double hoverMultiplier = 0.0;
    QTimer effectTimer;
    std::vector<ClickEffect> clickEffects;
    boost::optional<QColor> mouseEffectColor = boost::none;

    void onMouseEffectTimeout();
};

}  // namespace widgets
}  // namespace chatterino
