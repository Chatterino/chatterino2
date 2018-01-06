#pragma once

#include "QString"

namespace chatterino {
namespace widgets {
class ScrollbarHighlight
{
public:
    enum Style : char { None, Default, Line };
    enum Color : char { Highlight };

    ScrollbarHighlight();
    ScrollbarHighlight(Color _color, Style _style = Default);

    Color getColor() const;
    Style getStyle() const;
    bool isNull() const;

private:
    Color color;
    Style style;
};

}  // namespace widgets
}  // namespace chatterino
