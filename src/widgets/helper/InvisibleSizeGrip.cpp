// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/InvisibleSizeGrip.hpp"

namespace chatterino {

InvisibleSizeGrip::InvisibleSizeGrip(QWidget *parent)
    : QSizeGrip(parent)
{
    // required on Windows to prevent this from being ignored when dragging
    this->setMouseTracking(true);
}

void InvisibleSizeGrip::paintEvent(QPaintEvent *event)
{
}

}  // namespace chatterino
