#include "widgets/helper/TrimRegExpValidator.hpp"

namespace chatterino {

TrimRegExpValidator::TrimRegExpValidator(const QRegularExpression &re,
                                         QObject *parent)
    : QRegularExpressionValidator(re, parent)
{
}

QValidator::State TrimRegExpValidator::validate(QString &input, int &pos) const
{
    input = input.trimmed();
    return QRegularExpressionValidator::validate(input, pos);
}

}  // namespace chatterino
