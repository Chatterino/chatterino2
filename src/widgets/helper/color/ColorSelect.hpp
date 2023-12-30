#pragma once

#include "widgets/helper/color/AlphaSlider.hpp"
#include "widgets/helper/color/HueSlider.hpp"
#include "widgets/helper/color/SBCanvas.hpp"

#include <QBoxLayout>
#include <QWidget>

namespace chatterino {

class ColorSelect : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    ColorSelect(QColor color = {}, QWidget *parent = nullptr);

    QColor color() const;

signals:
    void colorChanged(QColor color);

public slots:
    void setColor(const QColor &color);

private:
    QColor currentColor_;
    int hue_ = 0;
    int saturation_ = 0;
    int brightness_ = 0;

    SBCanvas sbCanvas_;
    HueSlider hueSlider_;
    AlphaSlider alphaSlider_;
    QVBoxLayout rows_;

    void connectWidgets();

signals:
    void pickerColorChanged(QColor color);
    void detailsColorChanged(QColor color);
};

}  // namespace chatterino
