// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/buttons/Button.hpp"

namespace chatterino {

/// @brief A button with a #dim() setting for controlling the opacity of its
/// content.
///
/// This button doesn't paint anything.
///
/// @sa #currentContentOpacity()
class DimButton : public Button
{
public:
    enum class Dim : std::uint8_t {
        /// Fully opaque (100% opcaity)
        None,
        /// Slightly transparent (70% opacity)
        Some,
        /// Almost transparent (15% opacity)
        Lots,
    };

    DimButton(BaseWidget *parent = nullptr);

    /// Returns the current dim level.
    [[nodiscard]] Dim dim() const noexcept;

    /// Setter for #dim()
    void setDim(Dim value);

    /// Returns the current opacity based on the current dim level.
    [[nodiscard]] qreal currentContentOpacity() const noexcept;

private:
    Dim dim_ = Dim::Some;
};

}  // namespace chatterino
