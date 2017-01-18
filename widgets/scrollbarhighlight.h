#ifndef SCROLLBARHIGHLIGHT_H
#define SCROLLBARHIGHLIGHT_H

#include "QString"

namespace chatterino {
namespace widgets {

class ScrollBarHighlight
{
public:
    enum Style { Default, Left, Right, SingleLine };

    ScrollBarHighlight(float getPosition, int getColorIndex,
                       Style getStyle = Default, QString tag = "");

    Style
    getStyle()
    {
        return style;
    }

    float
    getPosition()
    {
        return position;
    }

    int
    getColorIndex()
    {
        return colorIndex;
    }

    QString
    getTag()
    {
        return tag;
    }

    ScrollBarHighlight *next;

private:
    Style style;
    float position;
    int colorIndex;
    QString tag;
};
}
}

#endif  // SCROLLBARHIGHLIGHT_H
