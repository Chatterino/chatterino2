// SPDX-FileCopyrightText: 2020 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BasePopup.hpp"

namespace chatterino {

class ColorPickerDialog : public BasePopup
{
    Q_OBJECT

public:
    ColorPickerDialog(QColor color, QWidget *parent);

    QColor color() const;

Q_SIGNALS:
    void colorChanged(QColor color);
    void colorConfirmed(QColor color);

public Q_SLOTS:
    void setColor(const QColor &color);

private:
    QColor color_;
};

}  // namespace chatterino
