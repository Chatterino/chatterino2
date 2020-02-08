#include "widgets/helper/ScrollbarHighlight.hpp"

#include "Application.hpp"
#include "singletons/Theme.hpp"
#include "widgets/Scrollbar.hpp"

namespace chatterino {

ScrollbarHighlight::ScrollbarHighlight()
    : color_(std::make_shared<QColor>())
    , style_(Style::None)
{
}

ScrollbarHighlight::ScrollbarHighlight(const std::shared_ptr<QColor> color,
                                       Style style)
    : color_(color)
    , style_(style)
{
}

QColor ScrollbarHighlight::getColor() const
{
    return *this->color_;
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
