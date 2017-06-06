#ifndef SCROLLBARHIGHLIGHT_H
#define SCROLLBARHIGHLIGHT_H

#include "QString"

namespace chatterino {
namespace widgets {

class ScrollBarHighlight
{
public:
    enum Style { Default, Left, Right, SingleLine };

    ScrollBarHighlight(float getPosition, int getColorIndex, Style getStyle = Default,
                       QString _tag = "");

    Style getStyle()
    {
        return _style;
    }

    float getPosition()
    {
        return _position;
    }

    int getColorIndex()
    {
        return _colorIndex;
    }

    QString getTag()
    {
        return _tag;
    }

    ScrollBarHighlight *next = nullptr;

private:
    Style _style;
    float _position;
    int _colorIndex;
    QString _tag;
};

}  // namespace widgets
}  // namespace chatterino

#endif  // SCROLLBARHIGHLIGHT_H
