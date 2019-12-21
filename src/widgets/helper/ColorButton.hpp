#pragma once

namespace chatterino {

class ColorButton : public QPushButton
{
public:
    ColorButton(const QColor &color);

    const QColor &color() const;

    void setColor(QColor color);

private:
    QColor color_;
};

}   // namespace chatterino
