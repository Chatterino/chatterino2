#pragma once

#include "widgets/buttons/Button.hpp"

#include <QWidget>

#include <optional>

namespace chatterino {

/// A button displaying custom-drawn primitive symbols.
class DrawnButton : public Button
{
    Q_OBJECT

public:
    enum class Symbol : std::uint8_t {
        /// +
        ///
        /// Default values:
        ///  Padding: 2px
        ///  Thickness: 1px
        Plus,
    };

    struct Options {
        /// The horizontal and vertical padding, in unscaled pixels, that will be applied when drawing the symbol
        std::optional<int> padding;

        /// The base line thickness that will be used when drawing the symbol.
        ///
        /// Each symbol might use the thickness slightly different
        std::optional<int> thickness;

        std::optional<QColor> background;
        std::optional<QColor> backgroundHover;

        std::optional<QColor> foreground;
        std::optional<QColor> foregroundHover;
    };

    DrawnButton(Symbol symbol_, Options options_, BaseWidget *parent);

    /// Update the override options for this button
    void setOptions(Options options_);

protected:
    void themeChangedEvent() override;

    void mouseOverUpdated() override;

    void paintContent(QPainter &painter) override;

private:
    /// Returns the padding (horizontal and vertical) to be used when drawing this button, taking the widget scale into consideration.
    ///
    /// Prioritizes the user-defined options over the symbol-defined options.
    int getPadding() const;

    /// Returns the line thickness to be used when drawing this button, taking the widget scale into consideration.
    ///
    /// Prioritizes the user-defined options over the symbol-defined options.
    int getThickness() const;

    /// Returns the background to be used when drawing this button.
    ///
    /// The color may be invalid, meaning no background is drawn.
    ///
    /// Prioritizes the user-defined options over the symbol-defined options.
    QColor getBackground() const;

    /// Returns the background to be used when drawing this button, and the user is hovering over it.
    ///
    /// The color may be invalid, meaning no background is drawn.
    ///
    /// Prioritizes the user-defined options over the symbol-defined options.
    QColor getBackgroundHover() const;

    /// Returns the foreground color to be used when drawing this button.
    ///
    /// The color must not be invalid.
    ///
    /// Prioritizes the user-defined options over the symbol-defined options.
    QColor getForeground() const;

    /// Returns the foreground color to be used when drawing this button, and the user is hovering over it.
    ///
    /// The color must not be invalid.
    ///
    /// Prioritizes the user-defined options over the symbol-defined options.
    QColor getForegroundHover() const;

    /// The options defined by the consumer / class wanting to draw this button
    Options options;

    /// The default options defined by the symbol
    Options symbolOptions;

    const Symbol symbol;
};

}  // namespace chatterino
