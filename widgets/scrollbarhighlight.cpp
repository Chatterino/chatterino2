#include "widgets/scrollbarhighlight.h"
#include "colorscheme.h"

namespace chatterino {
namespace widgets {

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
}
}
