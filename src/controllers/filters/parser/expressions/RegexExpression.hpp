#pragma once

#include "controllers/filters/parser/Types.hpp"
#include "controllers/filters/parser/expressions/Expression.hpp"

#include <QRegularExpression>

namespace filterparser {

class RegexExpression : public Expression
{
public:
    RegexExpression(QString regex, bool caseInsensitive);

    QVariant execute(const ContextMap &context) const override;
    PossibleType returnType() const override;
    bool validateTypes(TypeValidator &validator) const override;
    QString debug() const override;
    QString filterString() const override;

private:
    QString regexString_;
    bool caseInsensitive_;
    QRegularExpression regex_;
};

}  // namespace filterparser
