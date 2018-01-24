#include "rippleeffectbutton.hpp"

#include <QDebug>
#include <QPainter>

#include "singletons/thememanager.hpp"

namespace chatterino {
namespace widgets {

RippleEffectButton::RippleEffectButton(BaseWidget *parent)
    : BaseWidget(parent)
    , pixmap(nullptr)
{
    connect(&effectTimer, &QTimer::timeout, this, &RippleEffectButton::onMouseEffectTimeout);

    this->effectTimer.setInterval(20);
    this->effectTimer.start();
}

void RippleEffectButton::setMouseEffectColor(boost::optional<QColor> color)
{
    this->mouseEffectColor = color;
}

void RippleEffectButton::setPixmap(const QPixmap *_pixmap)
{
    this->pixmap = const_cast<QPixmap *>(_pixmap);
    this->update();
}

const QPixmap *RippleEffectButton::getPixmap() const
{
    return this->pixmap;
}

void RippleEffectButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    this->fancyPaint(painter);

    if (this->pixmap != nullptr) {
        QRect rect = this->rect();
        int xD = 6 * this->getDpiMultiplier();

        rect.moveLeft(xD);
        rect.setRight(rect.right() - xD - xD);
        rect.moveTop(xD);
        rect.setBottom(rect.bottom() - xD - xD);

        painter.drawPixmap(rect, *this->pixmap);
    }
}

void RippleEffectButton::fancyPaint(QPainter &painter)
{
    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor c;

    if (this->mouseEffectColor) {
        c = this->mouseEffectColor.get();
    } else {
        c = this->themeManager.isLightTheme() ? QColor(0, 0, 0) : QColor(255, 255, 255);
    }

    if (this->hoverMultiplier > 0) {
        QRadialGradient gradient(mousePos.x(), mousePos.y(), 50, mousePos.x(), mousePos.y());

        gradient.setColorAt(
            0, QColor(c.red(), c.green(), c.blue(), (int)(24 * this->hoverMultiplier)));
        gradient.setColorAt(
            1, QColor(c.red(), c.green(), c.blue(), (int)(12 * this->hoverMultiplier)));

        painter.fillRect(this->rect(), gradient);
    }

    for (auto effect : this->clickEffects) {
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

void RippleEffectButton::enterEvent(QEvent *)
{
    this->mouseOver = true;
}

void RippleEffectButton::leaveEvent(QEvent *)
{
    this->mouseOver = false;
}

void RippleEffectButton::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->clickEffects.push_back(ClickEffect(event->pos()));

    this->mouseDown = true;
}

void RippleEffectButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->mouseDown = false;

    if (this->rect().contains(event->pos())) {
        emit clicked();
    }
}

void RippleEffectButton::mouseMoveEvent(QMouseEvent *event)
{
    this->mousePos = event->pos();
}

void RippleEffectButton::onMouseEffectTimeout()
{
    bool performUpdate = false;

    if (selected) {
        if (this->hoverMultiplier != 0) {
            this->hoverMultiplier = std::max(0.0, this->hoverMultiplier - 0.1);
            performUpdate = true;
        }
    } else if (mouseOver) {
        if (this->hoverMultiplier != 1) {
            this->hoverMultiplier = std::min(1.0, this->hoverMultiplier + 0.5);
            performUpdate = true;
        }
    } else {
        if (this->hoverMultiplier != 0) {
            this->hoverMultiplier = std::max(0.0, this->hoverMultiplier - 0.3);
            performUpdate = true;
        }
    }

    if (this->clickEffects.size() != 0) {
        performUpdate = true;

        for (auto it = this->clickEffects.begin(); it != this->clickEffects.end();) {
            (*it).progress += mouseDown ? 0.02 : 0.07;

            if ((*it).progress >= 1.0) {
                it = this->clickEffects.erase(it);
            } else {
                it++;
            }
        }
    }

    if (performUpdate) {
        update();
    }
}

}  // namespace widgets
}  // namespace chatterino
