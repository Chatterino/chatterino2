#include "widgets/scrollbarhighlight.h"
#include "colorscheme.h"

namespace chatterino {
namespace widgets {

ScrollBarHighlight::ScrollBarHighlight(float position, int colorIndex, Style style, QString tag)
    : _position(position)
    , _colorIndex(std::max(0, std::min(ColorScheme::getInstance().HighlightColorCount, colorIndex)))
    , _style(style)
    , _tag(tag)
    , next(NULL)
{
}
}
}
