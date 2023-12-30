#pragma once

#include "widgets/BasePopup.hpp"

namespace chatterino {

class ColorPickerDialog : public BasePopup
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    ColorPickerDialog(QColor color = {}, QWidget *parent = nullptr);

    QColor color() const;

signals:
    void colorChanged(QColor color);
    void colorConfirmed(QColor color);

public slots:
    void setColor(const QColor &color);

private:
    QColor color_;
};

}  // namespace chatterino
