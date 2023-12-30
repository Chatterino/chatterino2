#include "widgets/helper/color/SBCanvas.hpp"

namespace {

constexpr int PICKER_WIDTH = 256;
constexpr int PICKER_HEIGHT = 256;

}  // namespace

namespace chatterino {

SBCanvas::SBCanvas(QColor color, QWidget *parent)
    : QWidget(parent)
{
    this->updateColor(color);
    this->setSizePolicy({QSizePolicy::Fixed, QSizePolicy::Fixed});
}

void SBCanvas::updateColor(const QColor &color)
{
    color.getHsv(&this->hue_, &this->saturation_, &this->brightness_);
    if (this->hue_ < 0)
    {
        this->hue_ = 0;
    }
    this->update();
}

void SBCanvas::setHue(int hue)
{
    this->hue_ = hue;
    this->updatePixmap();
    this->repaint();
}

int SBCanvas::saturation() const
{
    return this->saturation_;
}

int SBCanvas::brightness() const
{
    return this->brightness_;
}

QSize SBCanvas::sizeHint() const
{
    return {PICKER_WIDTH, PICKER_HEIGHT};
}

void SBCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    this->updatePixmap();
}

int SBCanvas::xPosToSaturation(int xPos) const
{
    return (xPos * 255) / this->width();
}

int SBCanvas::yPosToBrightness(int yPos) const
{
    return 255 - (yPos * 255) / this->height();
}

void SBCanvas::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        this->trackingMouseEvents_ = true;
        this->updateFromEvent(event);
        event->accept();
        this->setFocus(Qt::FocusReason::MouseFocusReason);
    }
}
void SBCanvas::mouseMoveEvent(QMouseEvent *event)
{
    if (this->trackingMouseEvents_)
    {
        this->updateFromEvent(event);
        event->accept();
    }
}
void SBCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    if (this->trackingMouseEvents_ &&
        event->buttons().testFlag(Qt::MouseButton::LeftButton))
    {
        this->updateFromEvent(event);
        this->trackingMouseEvents_ = false;
        event->accept();
    }
}

void SBCanvas::updateFromEvent(QMouseEvent *event)
{
    auto clampedX = std::clamp(event->pos().x(), 0, this->width());
    auto clampedY = std::clamp(event->pos().y(), 0, this->height());
    this->setSaturation(this->xPosToSaturation(clampedX));
    this->setBrightness(this->yPosToBrightness(clampedY));
}

void SBCanvas::updatePixmap()
{
    int w = this->width();
    int h = this->height();
    QImage img(w, h, QImage::Format_RGB32);
    uint *pixel = (uint *)img.scanLine(0);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            QColor c = QColor::fromHsv(this->hue_, this->xPosToSaturation(x),
                                       this->yPosToBrightness(y));
            *pixel = c.rgb();
            pixel++;
        }
    }
    this->gradientPixmap_ = QPixmap::fromImage(img);
}

void SBCanvas::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (this->gradientPixmap_.isNull())
    {
        this->updatePixmap();
    }

    painter.drawPixmap(this->rect().topLeft(), this->gradientPixmap_);

    QPoint circ = {(this->saturation() * this->width()) / 256,
                   ((255 - this->brightness()) * this->height()) / 256};
    auto circleColor = this->brightness() >= 128 ? 50 : 200;
    painter.setPen({QColor(circleColor, circleColor, circleColor), 2});
    painter.setBrush(Qt::transparent);
    painter.drawEllipse(circ, 5, 5);
}

void SBCanvas::setSaturation(int saturation)
{
    if (this->saturation_ == saturation)
    {
        return;
    }
    this->saturation_ = saturation;
    this->update();
    emit saturationChanged(saturation);
}

void SBCanvas::setBrightness(int brightness)
{
    if (this->brightness_ == brightness)
    {
        return;
    }
    this->brightness_ = brightness;
    this->update();
    emit brightnessChanged(brightness);
}

}  // namespace chatterino
