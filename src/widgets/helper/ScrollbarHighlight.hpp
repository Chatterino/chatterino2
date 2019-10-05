#pragma once

namespace chatterino {

class ScrollbarHighlight
{
public:
    enum Style : char { None, Default, Line };
    enum class Type { Highlight, Subscription, Invalid };

    /**
     * @brief Constructs an invalid ScrollbarHighlight.
     *
     * A highlight constructed this way will not show on the scrollbar.
     * For these, use the static ScrollbarHighlight::newSubscription and
     * ScrollbarHighlight::newHighlight methods.
     */
    ScrollbarHighlight();

    /**
     * @brief Constructs a new ScrollbarHighlight for subscriptions.
     * 
     * Use this method to obtain a ScrollbarHighlight that represents a
     * subscription message in chat. It will respect theme changes.
     *
     * @param style the style to use for the constructed ScrollbarHighlight
     */
    static ScrollbarHighlight newSubscription(Style style = Default);

    /**
     * @brief Constructs a new ScrollbarHighlight for highlighted messages.
     *
     * Use this method to obtain a ScrollbarHighlight that represents a
     * highlighted message in chat.
     *
     * @param color the color to use for the constructed ScrollbarHighlight
     * @param style the style to use for the constructed ScrollbarHighlight
     */
    static ScrollbarHighlight newHighlight(const QColor &color,
                                           Style style = Default);

    const QColor &getColor() const;
    Style getStyle() const;
    Type getType() const;
    bool isNull() const;

private:
    /**
     * This constructor is private so no invalid parameter combination gets
     * passed. For example, passing `Type::Subscription` and a custom color is
     * not allowed.
     *
     * For extern object creation, use the static
     * `ScrollbarHighlight::newSubscription` and
     * `ScrollbarHighlight::newHighlight` methods.
     */
    ScrollbarHighlight(Type type, const QColor &color, Style style);

    Type type_;
    QColor color_;
    Style style_;
};

}  // namespace chatterino
