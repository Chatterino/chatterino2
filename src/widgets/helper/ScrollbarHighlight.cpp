#include "widgets/helper/ScrollbarHighlight.hpp"
#include "singletons/Theme.hpp"
#include "widgets/Scrollbar.hpp"

namespace chatterino {

ScrollbarHighlight::ScrollbarHighlight()
    : color_(Color::Highlight)
    , style_(Style::None)
{
}

ScrollbarHighlight::ScrollbarHighlight(Color color, Style style)
    : color_(color)
    , style_(style)
{
}

ScrollbarHighlight::Color ScrollbarHighlight::getColor() const
{
    return this->color_;
}

ScrollbarHighlight::Style ScrollbarHighlight::getStyle() const
{
    return this->style_;
}

bool ScrollbarHighlight::isNull() const
{
    return this->style_ == None;
}

}  // namespace chatterino
