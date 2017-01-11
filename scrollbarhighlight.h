#ifndef SCROLLBARHIGHLIGHT_H
#define SCROLLBARHIGHLIGHT_H

#include "QString"

class ScrollBarHighlight
{
public:
    enum Style { Default, Left, Right, SingleLine };

    ScrollBarHighlight(float position, int colorIndex, Style style = Default,
                       QString tag = "");

    Style
    style()
    {
        return m_style;
    }

    float
    position()
    {
        return m_position;
    }

    int
    colorIndex()
    {
        return m_colorIndex;
    }

    QString
    getTag()
    {
        return m_tag;
    }

    ScrollBarHighlight *next;

private:
    Style m_style;
    float m_position;
    int m_colorIndex;
    QString m_tag;
};

#endif  // SCROLLBARHIGHLIGHT_H
