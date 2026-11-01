// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>

#include <memory>

namespace chatterino {

class ScrollbarHighlight
{
public:
    // TODO: Style is never anything but Default or None
    enum class Style : char {
        None,
        Default,
        Line,
    };

    /**
     * @brief Constructs an invalid ScrollbarHighlight.
     *
     * A highlight constructed this way will not show on the scrollbar.
     */
    ScrollbarHighlight();

    ScrollbarHighlight(const std::shared_ptr<QColor> &color,
                       Style style = Style::Default);

    QColor getColor() const;
    Style getStyle() const;
    bool isNull() const;

private:
    std::shared_ptr<QColor> color_;
    Style style_;
};

}  // namespace chatterino
