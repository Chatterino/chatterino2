#include "widgets/scrollbarhighlight.hpp"
#include "colorscheme.hpp"
#include "widgets/scrollbar.hpp"

namespace chatterino {
namespace widgets {

ScrollBarHighlight::ScrollBarHighlight(double _position, int _colorIndex, ScrollBar *parent,
                                       Style _style, QString _tag)
    : colorScheme(parent->colorScheme)
    , position(_position)
    , colorIndex(std::max(0, std::min(this->colorScheme.HighlightColorCount, _colorIndex)))
    , style(_style)
    , tag(_tag)
{
}

}  // namespace widgets
}  // namespace chatterino
