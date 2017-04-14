#include "fancybutton.h"

#include <QDebug>
#include <QPainter>

namespace  chatterino {
namespace  widgets {

FancyButton::FancyButton(QWidget *parent)
    : QWidget(parent)
    , _selected()
    , _mouseOver()
    , _mouseDown()
    , _mousePos()
    , _hoverMultiplier()
    , _effectTimer()
    , _mouseEffectColor(QColor(255, 255, 255))

{
    connect(&_effectTimer, &QTimer::timeout, this, &FancyButton::onMouseEffectTimeout);

    _effectTimer.setInterval(20);
    _effectTimer.start();
}

void FancyButton::setMouseEffectColor(QColor color)
{
    _mouseEffectColor = color;
}

void FancyButton::paintEvent(QPaintEvent *)
{
    QPainter painter;

    fancyPaint(painter);
}

void FancyButton::fancyPaint(QPainter &painter)
{
    QColor &c = _mouseEffectColor;

    if (_hoverMultiplier > 0) {
        QRadialGradient gradient(_mousePos.x(), _mousePos.y(), 50, _mousePos.x(), _mousePos.y());

        gradient.setColorAt(0, QColor(c.red(), c.green(), c.blue(), (int)(24 * _hoverMultiplier)));
        gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(), (int)(12 * _hoverMultiplier)));

        painter.fillRect(this->rect(), gradient);
    }

    for (auto effect : _clickEffects) {
        QRadialGradient gradient(effect.position.x(), effect.position.y(),
                                 effect.progress * (float)width() * 2, effect.position.x(),
                                 effect.position.y());

        gradient.setColorAt(
            0, QColor(c.red(), c.green(), c.blue(), (int)((1 - effect.progress) * 95)));
        gradient.setColorAt(
            0.9999, QColor(c.red(), c.green(), c.blue(), (int)((1 - effect.progress) * 95)));
        gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(), (int)(0)));

        painter.fillRect(this->rect(), gradient);
    }
}

void FancyButton::enterEvent(QEvent *)
{
    _mouseOver = true;
}

void FancyButton::leaveEvent(QEvent *)
{
    _mouseOver = false;
}

void FancyButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    _clickEffects.push_back(ClickEffect(event->pos()));

    _mouseDown = true;
}

void FancyButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    _mouseDown = false;
}

void FancyButton::mouseMoveEvent(QMouseEvent *event)
{
    _mousePos = event->pos();
}

void FancyButton::onMouseEffectTimeout()
{
    bool performUpdate = false;

    if (_selected) {
        if (_hoverMultiplier != 0) {
            _hoverMultiplier = std::max(0.0, _hoverMultiplier - 0.1);
            performUpdate = true;
        }
    } else if (_mouseOver) {
        if (_hoverMultiplier != 1) {
            _hoverMultiplier = std::min(1.0, _hoverMultiplier + 0.5);
            performUpdate = true;
        }
    } else {
        if (_hoverMultiplier != 0) {
            _hoverMultiplier = std::max(0.0, _hoverMultiplier - 0.3);
            performUpdate = true;
        }
    }

    if (_clickEffects.size() != 0) {
        performUpdate = true;

        for (auto it = _clickEffects.begin(); it != _clickEffects.end();) {
            (*it).progress += _mouseDown ? 0.02 : 0.07;

            if ((*it).progress >= 1.0) {
                it = _clickEffects.erase(it);
            } else {
                it++;
            }
        }
    }

    if (performUpdate) {
        update();
    }
}
}
}
