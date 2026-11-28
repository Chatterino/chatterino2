// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/color/ColorInput.hpp"

#include "Test.hpp"

#include <QColor>

using namespace chatterino;

TEST(ColorInput, ParsesShortHex)
{
    EXPECT_EQ(parseHexColor("#2468"), QColor(0x22, 0x44, 0x66, 0x88));
}
