// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/buttons/DimButton.hpp"

namespace chatterino {

/// @brief A (dimmable) button displaying a pixmap.
///
/// The pixmap is scaled to fit the button, ignoring any aspect ratio.
///
/// By default, a margin is added to the button (@see #marginEnabled()).
///
/// When possible, prefer using a `SvgButton` as it can scale its contents
/// to any size without blur and supports scaling and themeing.
///
/// @sa #setPixmap()
class PixmapButton : public DimButton
{
public:
    PixmapButton(BaseWidget *parent = nullptr);

    /// Returns the current, non-scaled pixmap
    [[nodiscard]] QPixmap pixmap() const;

    /// Setter for #pixmap()
    void setPixmap(const QPixmap &pixmap);

    /// @brief Returns true if this button has margin.
    ///
    /// The margin is dynamically calculated based on the UI-scale
    /// and the size of the pixmap.
    [[nodiscard]] bool marginEnabled() const noexcept;

    /// Setter for #marginEnabled()
    void setMarginEnabled(bool enableMargin);

protected:
    void paintContent(QPainter &painter) override;

private:
    QPixmap pixmap_;
    QPixmap resizedPixmap_;
    bool marginEnabled_ = true;
};

}  // namespace chatterino
