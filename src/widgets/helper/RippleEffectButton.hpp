#pragma once

#include <boost/optional.hpp>

#include "widgets/BaseWidget.hpp"

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
    void setPixmap(const QPixmap &pixmap_);
    const QPixmap &getPixmap() const;

    void setBorderColor(const QColor &color);
    const QColor &getBorderColor() const;

signals:
    void clicked();

protected:
    bool selected_ = false;
    bool mouseOver_ = false;
    bool mouseDown_ = false;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;

    void fancyPaint(QPainter &painter);

private:
    QColor borderColor_;
    QPixmap pixmap_;
    QPoint mousePos_;
    double hoverMultiplier_ = 0.0;
    QTimer effectTimer_;
    std::vector<ClickEffect> clickEffects_;
    boost::optional<QColor> mouseEffectColor_ = boost::none;

    void onMouseEffectTimeout();
};

}  // namespace widgets
}  // namespace chatterino
