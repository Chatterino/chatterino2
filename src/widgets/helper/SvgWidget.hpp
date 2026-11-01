// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QWidget>

class QSvgRenderer;

namespace chatterino {

/// Reimplementation of QSvgWidget without support for animations.
class SvgWidget : public QWidget
{
public:
    SvgWidget(QWidget *parent = nullptr);

    QSvgRenderer *renderer();

    QSize sizeHint() const override;

    void load(const QString &file);
    void load(const QByteArray &contents);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSvgRenderer *renderer_;
};

}  // namespace chatterino
