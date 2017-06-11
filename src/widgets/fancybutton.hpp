#pragma once

#include <QMouseEvent>
#include <QPainter>
#include <QPoint>
#include <QTimer>
#include <QWidget>

namespace chatterino {
namespace widgets {

class FancyButton : public QWidget
{
    struct ClickEffect {
        float progress;
        QPoint position;

        ClickEffect(QPoint position)
            : progress()
            , position(position)
        {
        }
    };

public:
    FancyButton(QWidget *parent = nullptr);

    void setMouseEffectColor(QColor color);

protected:
    void paintEvent(QPaintEvent *) override;
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void fancyPaint(QPainter &painter);

private:
    bool _selected;
    bool _mouseOver;
    bool _mouseDown;
    QPoint _mousePos;
    float _hoverMultiplier;
    QTimer _effectTimer;
    std::vector<ClickEffect> _clickEffects;
    QColor _mouseEffectColor;

    void onMouseEffectTimeout();
};

}  // namespace widgets
}  // namespace chatterino
