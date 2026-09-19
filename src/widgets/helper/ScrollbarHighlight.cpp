// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/ScrollbarHighlight.hpp"

namespace chatterino {

ScrollbarHighlight::ScrollbarHighlight()
    : color_(std::make_shared<QColor>())
    , style_(Style::None)
{
}

ScrollbarHighlight::ScrollbarHighlight(const std::shared_ptr<QColor> &color,
                                       Style style)
    : color_(color)
    , style_(style)
{
}

QColor ScrollbarHighlight::getColor() const
{
    assert(this->color_);
    return *this->color_;
}

ScrollbarHighlight::Style ScrollbarHighlight::getStyle() const
{
    return this->style_;
}

bool ScrollbarHighlight::isNull() const
{
    return this->style_ == Style::None || !this->color_;
}

}  // namespace chatterino
