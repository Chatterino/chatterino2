#include "widgets/helper/ScrollbarHighlight.hpp"

#include "Application.hpp"
#include "singletons/Theme.hpp"
#include "widgets/Scrollbar.hpp"

namespace chatterino {

ScrollbarHighlight::ScrollbarHighlight()
    : type_(Type::Invalid)
    , color_(QColor())
    , style_(Style::None)
{
}

ScrollbarHighlight ScrollbarHighlight::newSubscription(Style style)
{
    return ScrollbarHighlight(Type::Subscription, QColor(), style);
}

ScrollbarHighlight ScrollbarHighlight::newHighlight(const QColor &color,
                                                    Style style)
{
    return ScrollbarHighlight(Type::Highlight, color, style);
}

ScrollbarHighlight::ScrollbarHighlight(Type type, const QColor &color,
                                       Style style)
    : type_(type)
    , color_(color)
    , style_(style)
{
}

ScrollbarHighlight::Type ScrollbarHighlight::getType() const
{
    return this->type_;
}

const QColor &ScrollbarHighlight::getColor() const
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
