#include "widgets/helper/ScrollbarHighlight.hpp"
#include "singletons/ThemeManager.hpp"
#include "widgets/Scrollbar.hpp"

namespace chatterino {
namespace widgets {

ScrollbarHighlight::ScrollbarHighlight()
    : color(Color::Highlight)
    , style(Style::None)
{
}

ScrollbarHighlight::ScrollbarHighlight(Color _color, Style _style)
    : color(_color)
    , style(_style)
{
}

ScrollbarHighlight::Color ScrollbarHighlight::getColor() const
{
    return this->color;
}

ScrollbarHighlight::Style ScrollbarHighlight::getStyle() const
{
    return this->style;
}

bool ScrollbarHighlight::isNull() const
{
    return this->style == None;
}

}  // namespace widgets
}  // namespace chatterino
