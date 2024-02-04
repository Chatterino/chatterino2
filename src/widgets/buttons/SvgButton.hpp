#pragma once

#include "widgets/buttons/Button.hpp"

#include <QString>

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
    [[nodiscard]] SvgButton(const QString &unified,
                            BaseWidget *parent = nullptr)
        : SvgButton({.dark = unified, .light = unified}, parent)
    {
    }

    /// Returns the current source configuration.
    [[nodiscard]] Src source() const;

    /// Setter for #source()
    void setSource(Src source);

    /// @brief Setter for #source()
    ///
    /// This will set the same source file for both light and dark theme,
    /// effectively turning off automatic themeing.
    void setSource(const QString &unified);

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

    void paintContent(QPainter &painter) override;

private:
    [[nodiscard]] QString currentSvgPath() const;

    Src source_;
    QSvgRenderer *svg_;
    QSize padding_;
};

}  // namespace chatterino
