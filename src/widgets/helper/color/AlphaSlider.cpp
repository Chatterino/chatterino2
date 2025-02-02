#include "widgets/helper/color/AlphaSlider.hpp"

#include "widgets/helper/color/Checkerboard.hpp"

#include <QMouseEvent>
#include <QPainterPath>

namespace {

constexpr int SLIDER_WIDTH = 256;
constexpr int SLIDER_HEIGHT = 12;

}  // namespace

namespace chatterino {

AlphaSlider::AlphaSlider(QColor color, QWidget *parent)
    : QWidget(parent)
    , alpha_(color.alpha())
    , color_(color)
{
    this->setSizePolicy({QSizePolicy::Expanding, QSizePolicy::Fixed});
}

void AlphaSlider::setColor(QColor color)
{
    if (this->color_ == color)
    {
        return;
    }
    this->alpha_ = color.alpha();
    this->color_ = color;
    this->cachedPixmap_ = {};
    this->update();
}

int AlphaSlider::alpha() const
{
    return this->alpha_;
}

QSize AlphaSlider::sizeHint() const
{
    return {SLIDER_WIDTH, SLIDER_HEIGHT};
}

void AlphaSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    this->updatePixmap();
}

int AlphaSlider::xPosToAlpha(int xPos) const
{
    return (xPos * 255) / (this->width() - this->height());
}

void AlphaSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        this->trackingMouseEvents_ = true;
        this->updateFromEvent(event);
        this->setFocus(Qt::FocusReason::MouseFocusReason);
    }
}
void AlphaSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (this->trackingMouseEvents_)
    {
        this->updateFromEvent(event);
        event->accept();
    }
}
void AlphaSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->trackingMouseEvents_ &&
        event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        this->updateFromEvent(event);
        this->trackingMouseEvents_ = false;
        event->accept();
    }
}

void AlphaSlider::updateFromEvent(QMouseEvent *event)
{
    int cornerRadius = this->height() / 2;
    auto clampedX = std::clamp(event->pos().x(), cornerRadius,
                               this->width() - cornerRadius);
    this->setAlpha(this->xPosToAlpha(clampedX - cornerRadius));
}

void AlphaSlider::updatePixmap()
{
    this->cachedPixmap_ = QPixmap(this->size());
    this->cachedPixmap_.fill(Qt::transparent);
    QPainter painter(&this->cachedPixmap_);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal cornerRadius = (qreal)this->height() / 2.0;

    QPainterPath mask;
    mask.addRoundedRect(QRect({0, 0}, this->size()), cornerRadius,
                        cornerRadius);
    painter.setClipPath(mask);

    drawCheckerboard(painter, this->size(), this->height() / 2);

    QLinearGradient gradient(cornerRadius, 0.0,
                             (qreal)this->width() - cornerRadius, 0.0);
    QColor start = this->color_;
    QColor end = this->color_;
    start.setAlpha(0);
    end.setAlpha(255);

    gradient.setColorAt(0.0, start);
    gradient.setColorAt(1.0, end);

    painter.setPen({Qt::transparent, 0});
    painter.setBrush(gradient);
    painter.drawRect(QRect({0, 0}, this->size()));
}

void AlphaSlider::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (this->cachedPixmap_.isNull())
    {
        this->updatePixmap();
    }

    painter.drawPixmap(this->rect().topLeft(), this->cachedPixmap_);

    int cornerRadius = this->height() / 2;

    QPoint circ = {
        cornerRadius +
            (this->alpha() * (this->width() - 2 * cornerRadius)) / 255,
        cornerRadius};
    auto circleColor = 0;
    painter.setPen({QColor(circleColor, circleColor, circleColor), 2});
    auto opaqueBase = this->color_;
    opaqueBase.setAlpha(255);
    painter.setBrush(opaqueBase);
    painter.drawEllipse(circ, cornerRadius - 1, cornerRadius - 1);
}

void AlphaSlider::setAlpha(int alpha)
{
    if (this->alpha_ == alpha)
    {
        return;
    }
    this->alpha_ = alpha;
    this->color_.setAlpha(alpha);

    this->colorChanged(this->color_);
    this->update();
}

}  // namespace chatterino
