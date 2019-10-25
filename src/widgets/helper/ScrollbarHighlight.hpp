#pragma once

namespace chatterino {

class ScrollbarHighlight
{
public:
    enum Style : char { None, Default, Line };

    /**
     * @brief Constructs an invalid ScrollbarHighlight.
     *
     * A highlight constructed this way will not show on the scrollbar.
     * For these, use the static ScrollbarHighlight::newSubscription and
     * ScrollbarHighlight::newHighlight methods.
     */
    ScrollbarHighlight();

    ScrollbarHighlight(const std::shared_ptr<QColor> color,
                       Style style = Default);

    QColor getColor() const;
    Style getStyle() const;
    bool isNull() const;

private:
    std::shared_ptr<QColor> color_;
    Style style_;
};

}  // namespace chatterino
