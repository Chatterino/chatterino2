// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <type_traits>  // IWYU: pragma keep - included before QFlags to fix a bug on macOS Qt 6.7
// Keep this type_traits include above the QFlags include

#include <QFlags>
#include <QLabel>
#include <QMouseEvent>
#include <QWidget>

namespace chatterino {

class SignalLabel : public QLabel
{
    Q_OBJECT

public:
    explicit SignalLabel(QWidget *parent = nullptr, Qt::WindowFlags f = {});
    ~SignalLabel() override = default;

Q_SIGNALS:
    void mouseDoubleClick(QMouseEvent *ev);

    void leftMouseDown();
    void leftMouseUp();
    void mouseMove(QMouseEvent *event);

protected:
    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
};

}  // namespace chatterino
