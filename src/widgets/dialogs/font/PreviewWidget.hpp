// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QFont>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class PreviewWidget : public QWidget
{
public:
    PreviewWidget(const QFont &startFont, QWidget *parent = nullptr);

    void setFont(const QFont &font);

    void paintEvent(QPaintEvent *event) override;

private:
    QFont font;
};

}  // namespace chatterino
