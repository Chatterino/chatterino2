// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/color/ColorInput.hpp"

#include "Test.hpp"

#include <QColor>
#include <QLineEdit>
#include <QMetaObject>

using namespace chatterino;

TEST(ColorInput, ParsesShortHex)
{
    ColorInput input(QColor(0x000000));
    auto *hexInput = input.findChild<QLineEdit *>();
    ASSERT_NE(hexInput, nullptr);

    hexInput->setText("#2468");
    ASSERT_TRUE(QMetaObject::invokeMethod(hexInput, "editingFinished",
                                          Qt::DirectConnection));

    EXPECT_EQ(input.color(), QColor(0x22, 0x44, 0x66, 0x88));
}
