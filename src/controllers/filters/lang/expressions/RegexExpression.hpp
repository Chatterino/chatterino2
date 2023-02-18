#pragma once

#include "controllers/filters/lang/expressions/Expression.hpp"
#include "controllers/filters/lang/Types.hpp"

#include <QRegularExpression>

namespace chatterino::filters {

class RegexExpression : public Expression
{
public:
    RegexExpression(const QString &regex, bool caseInsensitive);

    QVariant execute(const ContextMap &context) const override;
    PossibleType synthesizeType(const TypingContext &context) const override;
    QString debug(const TypingContext &context) const override;
    QString filterString() const override;

private:
    QString regexString_;
    bool caseInsensitive_;
    QRegularExpression regex_;
};

}  // namespace chatterino::filters
