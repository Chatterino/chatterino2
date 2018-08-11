#pragma once

namespace chatterino {

class ScrollbarHighlight
{
public:
    enum Style : char { None, Default, Line };
    enum Color : char { Highlight, Subscription };

    ScrollbarHighlight();
    ScrollbarHighlight(Color color, Style style = Default);

    Color getColor() const;
    Style getStyle() const;
    bool isNull() const;

private:
    Color color_;
    Style style_;
};

}  // namespace chatterino
