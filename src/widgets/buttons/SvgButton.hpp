#pragma once

#include "widgets/buttons/Button.hpp"

#include <QColor>
#include <QString>

#include <optional>

class QSvgRenderer;

namespace chatterino {

/// @brief A button displaying SVGs.
///
/// The source can be specified per theme (dark/light)
/// and will get automatically updated when the theme changes.
///
/// To set the size, use #setInnerHeight(). The width is adjusted according
/// to the aspect ratio of the SVG. If the button should scale with the UI,
/// turn on auto-scaling (@see #setAutoScale()), which will scale the button
/// and padding according to the current UI-scale (linearly).
///
/// The button has #padding() around the SVG.
class SvgButton : public Button
{
    Q_OBJECT

public:
    /// Source configuration for the button
    struct Src {
        /// Source to load in a dark theme
        QString dark;
        /// Source to load in a light theme
        QString light;
    };

    [[nodiscard]] SvgButton(Src source, BaseWidget *parent = nullptr,
                            QSize padding = {6, 3});

    /// Returns the current source configuration.
    [[nodiscard]] Src source() const;

    /// Setter for #source()
    void setSource(Src source);

    /// Sets a custom color to render over the SVG.
    /// This allows you to change the color of a button to a solid color
    /// of your choice without using multiple SVG resources.
    ///
    /// Set to std::nullopt to not override the color.
    void setColor(std::optional<QColor> color);

    /// @brief Returns the padding inside the button.
    ///
    /// `width` is the padding applied horizontally (left and right).
    /// `height` is the padding applied vertically (top and bottom).
    ///
    /// By default, the button has a vertical padding of 3 and a horizontal padding of 6.
    [[nodiscard]] QSize padding() const;

    /// Setter for #padding()
    void setPadding(QSize padding);

protected:
    void themeChangedEvent() override;
    void scaleChangedEvent(float scale) override;
    void resizeEvent(QResizeEvent *e) override;

    void paintContent(QPainter &painter) override;

private:
    [[nodiscard]] QString currentSvgPath() const;

    void loadSource();

    Src source_;
    QSvgRenderer *svg_;
    QSize padding_;
    std::optional<QColor> color_;
};

}  // namespace chatterino
