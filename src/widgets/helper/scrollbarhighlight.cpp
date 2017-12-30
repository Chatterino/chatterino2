#include "widgets/helper/scrollbarhighlight.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/scrollbar.hpp"

namespace chatterino {
namespace widgets {

ScrollBarHighlight::ScrollBarHighlight(double _position, int _colorIndex, ScrollBar *parent,
                                       Style _style, QString _tag)
    : themeManager(parent->themeManager)
    , position(_position)
    , colorIndex(std::max(0, std::min(this->themeManager.HighlightColorCount, _colorIndex)))
    , style(_style)
    , tag(_tag)
{
}

}  // namespace widgets
}  // namespace chatterino
