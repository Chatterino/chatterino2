#pragma once

#include "QString"

namespace chatterino {

class ThemeManager;

namespace widgets {

class ScrollBar;

class ScrollBarHighlight
{
public:
    enum Style { Default, Left, Right, SingleLine };

    ScrollBarHighlight(double _position, int _colorIndex, ScrollBar *parent, Style _style = Default,
                       QString _tag = "");

    ThemeManager &themeManager;

    double getPosition()
    {
        return this->position;
    }

    int getColorIndex()
    {
        return this->colorIndex;
    }

    Style getStyle()
    {
        return this->style;
    }

    QString getTag()
    {
        return this->tag;
    }

    ScrollBarHighlight *next = nullptr;

private:
    double position;
    int colorIndex;
    Style style;
    QString tag;
};

}  // namespace widgets
}  // namespace chatterino
