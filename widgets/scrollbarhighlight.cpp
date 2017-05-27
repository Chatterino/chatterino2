#include "widgets/scrollbarhighlight.h"
#include "colorscheme.h"

namespace chatterino {
namespace widgets {

ScrollBarHighlight::ScrollBarHighlight(float position, int colorIndex, Style style, QString tag)
    : _style(style)
    , _position(position)
    , _colorIndex(std::max(0, std::min(ColorScheme::getInstance().HighlightColorCount, colorIndex)))
    , _tag(tag)
{
}

}  // namespace widgets
}  // namespace chatterino
