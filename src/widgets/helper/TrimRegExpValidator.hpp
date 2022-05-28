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
