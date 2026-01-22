// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QRegularExpression>
#include <QRegularExpressionValidator>

namespace chatterino {

class TrimRegExpValidator : public QRegularExpressionValidator
{
    Q_OBJECT

public:
    TrimRegExpValidator(const QRegularExpression &re,
                        QObject *parent = nullptr);

    QValidator::State validate(QString &input, int &pos) const override;
};

}  // namespace chatterino
