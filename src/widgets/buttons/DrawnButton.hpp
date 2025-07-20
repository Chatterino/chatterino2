#pragma once

#include "widgets/buttons/Button.hpp"

#include <QWidget>

#include <optional>

namespace chatterino {

class DrawnButton : public Button
{
    Q_OBJECT

public:
    enum class Type : std::uint8_t {
        Plus,
    };

    struct Options {
        int padding = 2;
        int thickness = 1;
    };

    DrawnButton(Type type_, Options options, BaseWidget *parent);

    void setBackground(QColor color);
    void setBackgroundHover(QColor color);
    void setForeground(QColor color);
    void setForegroundHover(QColor color);

protected:
    void mouseOverUpdated() override;

    void paintContent(QPainter &painter) override;

private:
    /// Returns the padding (horizontal and vertical) to be used when drawing this button, taking the widget scale into consideration.
    int getPadding() const;

    /// Returns the line thickness to be used when drawing this button, taking the widget scale into consideration.
    int getThickness() const;

    QColor background;
    QColor backgroundHover;
    QColor foreground;
    QColor foregroundHover;

    /// The padding without taking the widget scale into consideration.
    const int basePadding;

    /// The line thickness without taking the widget scale into consideration.
    const int baseThickness;
    const Type type = Type::Plus;
};

}  // namespace chatterino
