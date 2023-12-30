#include "widgets/helper/color/ColorSelect.hpp"

namespace chatterino {

ColorSelect::ColorSelect(QColor color, QWidget *parent)
    : QWidget(parent)
    , currentColor_(color)
    , sbCanvas_(color)
    , hueSlider_(color)
    , alphaSlider_(color)

{
    color.getHsv(&this->hue_, &this->saturation_, &this->brightness_);

    this->rows_.addWidget(&this->sbCanvas_, 0, Qt::AlignHCenter);
    this->rows_.addWidget(&this->hueSlider_);
    this->rows_.addWidget(&this->alphaSlider_);
    this->setLayout(&this->rows_);
    this->connectWidgets();
}

void ColorSelect::connectWidgets()
{
    const auto applyColor = [this]() {
        emit colorChanged(this->currentColor_);
    };

    const auto updateCurrentColor = [this, applyColor]() {
        this->currentColor_ =
            QColor::fromHsv(this->hue_, this->saturation_, this->brightness_,
                            this->currentColor_.alpha());
        applyColor();
    };

    QObject::connect(&this->hueSlider_, &HueSlider::hueChanged,
                     [this, updateCurrentColor](int hue) {
                         this->hue_ = hue;
                         this->sbCanvas_.setHue(hue);
                         updateCurrentColor();
                         this->alphaSlider_.updateColor(this->currentColor_);
                     });
    QObject::connect(&this->alphaSlider_, &AlphaSlider::alphaChanged,
                     [this, applyColor](int alpha) {
                         this->currentColor_.setAlpha(alpha);
                         applyColor();
                     });
    const auto updateAfterSbChanged = [this, updateCurrentColor]() {
        updateCurrentColor();
        this->alphaSlider_.updateColor(this->currentColor_);
    };
    QObject::connect(&this->sbCanvas_, &SBCanvas::saturationChanged,
                     [this, updateAfterSbChanged](int saturation) {
                         this->saturation_ = saturation;
                         updateAfterSbChanged();
                     });
    QObject::connect(&this->sbCanvas_, &SBCanvas::brightnessChanged,
                     [this, updateAfterSbChanged](int brightness) {
                         this->brightness_ = brightness;
                         updateAfterSbChanged();
                     });
}

QColor ColorSelect::color() const
{
    return this->currentColor_;
}

void ColorSelect::setColor(const QColor &color)
{
    if (this->currentColor_ == color)
    {
        return;
    }
    this->currentColor_ = color;
    color.getHsv(&this->hue_, &this->saturation_, &this->brightness_);
    if (this->hue_ < 0)
    {
        this->hue_ = 0;
    }

    this->sbCanvas_.updateColor(color);
    this->hueSlider_.updateColor(color);
    this->alphaSlider_.updateColor(color);
}

}  // namespace chatterino
