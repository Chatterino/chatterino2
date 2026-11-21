// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QSizeGrip>

namespace chatterino {

class InvisibleSizeGrip : public QSizeGrip
{
public:
    explicit InvisibleSizeGrip(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

}  // namespace chatterino
