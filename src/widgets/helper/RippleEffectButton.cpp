#include "RippleEffectButton.hpp"

#include <QDebug>
#include <QPainter>

#include "singletons/Theme.hpp"

namespace chatterino {

RippleEffectButton::RippleEffectButton(BaseWidget *parent)
    : BaseWidget(parent)
{
    connect(&effectTimer_, &QTimer::timeout, this, &RippleEffectButton::onMouseEffectTimeout);

    this->effectTimer_.setInterval(20);
    this->effectTimer_.start();

    this->setMouseTracking(true);
}

void RippleEffectButton::setMouseEffectColor(boost::optional<QColor> color)
{
    this->mouseEffectColor_ = color;
}

void RippleEffectButton::setPixmap(const QPixmap &_pixmap)
{
    this->pixmap_ = _pixmap;
    this->update();
}

const QPixmap &RippleEffectButton::getPixmap() const
{
    return this->pixmap_;
}

void RippleEffectButton::setDim(bool value)
{
    this->dimPixmap_ = value;

    this->update();
}

bool RippleEffectButton::getDim() const
{
    return this->dimPixmap_;
}

void RippleEffectButton::setEnable(bool value)
{
    this->enabled_ = value;

    this->update();
}

bool RippleEffectButton::getEnable() const
{
    return this->enabled_;
}

qreal RippleEffectButton::getCurrentDimAmount() const
{
    return this->dimPixmap_ && !this->mouseOver_ ? 0.7 : 1;
}

void RippleEffectButton::setBorderColor(const QColor &color)
{
    this->borderColor_ = color;

    this->update();
}

const QColor &RippleEffectButton::getBorderColor() const
{
    return this->borderColor_;
}

void RippleEffectButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!this->pixmap_.isNull()) {
        if (!this->mouseOver_ && this->dimPixmap_ && this->enabled_) {
            painter.setOpacity(this->getCurrentDimAmount());
        }

        QRect rect = this->rect();
        int s = int(6 * this->getScale());

        rect.moveLeft(s);
        rect.setRight(rect.right() - s - s);
        rect.moveTop(s);
        rect.setBottom(rect.bottom() - s - s);

        painter.drawPixmap(rect, this->pixmap_);

        painter.setOpacity(1);
    }

    this->fancyPaint(painter);

    if (this->borderColor_.isValid()) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(this->borderColor_);
        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }
}

void RippleEffectButton::fancyPaint(QPainter &painter)
{
    if (!this->enabled_) {
        return;
    }

    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor c;

    if (this->mouseEffectColor_) {
        c = this->mouseEffectColor_.get();
    } else {
        c = this->theme->isLightTheme() ? QColor(0, 0, 0) : QColor(255, 255, 255);
    }

    if (this->hoverMultiplier_ > 0) {
        QRadialGradient gradient(QPointF(mousePos_), this->width() / 2);

        gradient.setColorAt(0,
                            QColor(c.red(), c.green(), c.blue(), int(50 * this->hoverMultiplier_)));
        gradient.setColorAt(1,
                            QColor(c.red(), c.green(), c.blue(), int(40 * this->hoverMultiplier_)));

        painter.fillRect(this->rect(), gradient);
    }

    for (auto effect : this->clickEffects_) {
        QRadialGradient gradient(effect.position.x(), effect.position.y(),
                                 effect.progress * qreal(width()) * 2, effect.position.x(),
                                 effect.position.y());

        gradient.setColorAt(0,
                            QColor(c.red(), c.green(), c.blue(), int((1 - effect.progress) * 95)));
        gradient.setColorAt(0.9999,
                            QColor(c.red(), c.green(), c.blue(), int((1 - effect.progress) * 95)));
        gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(), int(0)));

        painter.fillRect(this->rect(), gradient);
    }
}

void RippleEffectButton::enterEvent(QEvent *)
{
    this->mouseOver_ = true;
}

void RippleEffectButton::leaveEvent(QEvent *)
{
    this->mouseOver_ = false;
}

void RippleEffectButton::mousePressEvent(QMouseEvent *event)
{
    if (!this->enabled_) {
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->clickEffects_.push_back(ClickEffect(event->pos()));

    this->mouseDown_ = true;

    emit this->leftMousePress();
}

void RippleEffectButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (!this->enabled_) {
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    this->mouseDown_ = false;

    if (this->rect().contains(event->pos())) {
        emit clicked();
    }
}

void RippleEffectButton::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->enabled_) {
        return;
    }

    this->mousePos_ = event->pos();

    this->update();
}

void RippleEffectButton::onMouseEffectTimeout()
{
    bool performUpdate = false;

    if (selected_) {
        if (this->hoverMultiplier_ != 0) {
            this->hoverMultiplier_ = std::max(0.0, this->hoverMultiplier_ - 0.1);
            performUpdate = true;
        }
    } else if (mouseOver_) {
        if (this->hoverMultiplier_ != 1) {
            this->hoverMultiplier_ = std::min(1.0, this->hoverMultiplier_ + 0.5);
            performUpdate = true;
        }
    } else {
        if (this->hoverMultiplier_ != 0) {
            this->hoverMultiplier_ = std::max(0.0, this->hoverMultiplier_ - 0.3);
            performUpdate = true;
        }
    }

    if (this->clickEffects_.size() != 0) {
        performUpdate = true;

        for (auto it = this->clickEffects_.begin(); it != this->clickEffects_.end();) {
            it->progress += mouseDown_ ? 0.02 : 0.07;

            if (it->progress >= 1.0) {
                it = this->clickEffects_.erase(it);
            } else {
                it++;
            }
        }
    }

    if (performUpdate) {
        update();
    }
}

}  // namespace chatterino
