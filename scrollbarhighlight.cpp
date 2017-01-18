#include "scrollbarhighlight.h"
#include "colorscheme.h"

ScrollBarHighlight::ScrollBarHighlight(float position, int colorIndex,
                                       Style style, QString tag)
    : position(position)
    , colorIndex(std::max(
          0, std::min(ColorScheme::instance().HighlightColorCount, colorIndex)))
    , style(style)
    , tag(tag)
    , next(NULL)
{
}
