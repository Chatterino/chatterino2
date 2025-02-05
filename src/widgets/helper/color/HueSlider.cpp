#include "widgets/helper/color/HueSlider.hpp"

#include <QMouseEvent>
#include <QPainter>

namespace {

constexpr int SLIDER_WIDTH = 256;
constexpr int SLIDER_HEIGHT = 12;

}  // namespace

namespace chatterino {

HueSlider::HueSlider(QColor color, QWidget *parent)
    : QWidget(parent)
{
    this->setColor(color);
    this->setSizePolicy({QSizePolicy::Expanding, QSizePolicy::Fixed});
}

void HueSlider::setColor(QColor color)
{
    if (this->color_ == color)
    {
        return;
    }
    this->color_ = color.toHsv();

    auto hue = std::max(this->color_.hue(), 0);
    if (this->hue_ == hue)
    {
        return;
    }

    this->hue_ = hue;
    this->update();
}

int HueSlider::hue() const
{
    return this->hue_;
}

QSize HueSlider::sizeHint() const
{
    return {SLIDER_WIDTH, SLIDER_HEIGHT};
}

void HueSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    this->updatePixmap();
}

int HueSlider::xPosToHue(int xPos) const
{
    return (xPos * 359) / (this->width() - this->height());
}

void HueSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        this->trackingMouseEvents_ = true;
        this->updateFromEvent(event);
        event->accept();
        this->setFocus(Qt::FocusReason::MouseFocusReason);
    }
}
void HueSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (this->trackingMouseEvents_)
    {
        this->updateFromEvent(event);
        event->accept();
    }
}
void HueSlider::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->trackingMouseEvents_ &&
        event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        this->updateFromEvent(event);
        this->trackingMouseEvents_ = false;
        event->accept();
    }
}

void HueSlider::updateFromEvent(QMouseEvent *event)
{
    int cornerRadius = this->height() / 2;
    auto clampedX = std::clamp(event->pos().x(), cornerRadius,
                               this->width() - cornerRadius);
    this->setHue(this->xPosToHue(clampedX - cornerRadius));
}

void HueSlider::updatePixmap()
{
    constexpr int nStops = 10;
    constexpr auto nStopsF = (qreal)nStops;

    this->gradientPixmap_ = QPixmap(this->size());
    this->gradientPixmap_.fill(Qt::transparent);
    QPainter painter(&this->gradientPixmap_);
    painter.setRenderHint(QPainter::Antialiasing);

    qreal cornerRadius = (qreal)this->height() / 2.0;

    QLinearGradient gradient(cornerRadius, 0.0,
                             (qreal)this->width() - cornerRadius, 0.0);
    for (int i = 0; i <= nStops; i++)
    {
        gradient.setColorAt(
            (qreal)i / nStopsF,
            QColor::fromHsv(std::min((i * 360) / nStops, 359), 255, 255));
    }
    painter.setPen({Qt::transparent, 0});
    painter.setBrush(gradient);
    painter.drawRoundedRect(QRect({0, 0}, this->size()), cornerRadius,
                            cornerRadius);
}

void HueSlider::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (this->gradientPixmap_.isNull())
    {
        this->updatePixmap();
    }

    painter.drawPixmap(this->rect().topLeft(), this->gradientPixmap_);

    int cornerRadius = this->height() / 2;

    QPoint circ = {
        cornerRadius + (this->hue() * (this->width() - 2 * cornerRadius)) / 360,
        cornerRadius};
    auto circleColor = 0;
    painter.setPen({QColor(circleColor, circleColor, circleColor), 2});
    painter.setBrush(QColor::fromHsv(this->hue(), 255, 255));
    painter.drawEllipse(circ, cornerRadius - 1, cornerRadius - 1);
}

void HueSlider::setHue(int hue)
{
    if (this->hue_ == hue)
    {
        return;
    }
    this->hue_ = hue;
    // ugh
    int h{};
    int s{};
    int v{};
    int a{};
    this->color_.getHsv(&h, &s, &v, &a);
    this->color_.setHsv(this->hue_, s, v, a);

    this->colorChanged(this->color_);
    this->update();
}

}  // namespace chatterino
