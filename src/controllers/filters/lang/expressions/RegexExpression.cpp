#include "controllers/filters/lang/expressions/RegexExpression.hpp"

namespace chatterino::filters {

RegexExpression::RegexExpression(const QString &regex, bool caseInsensitive)
    : regexString_(regex)
    , caseInsensitive_(caseInsensitive)
    , regex_(QRegularExpression(
          regex, caseInsensitive ? QRegularExpression::CaseInsensitiveOption
                                 : QRegularExpression::NoPatternOption)){};

QVariant RegexExpression::execute(const ContextMap & /*context*/) const
{
    return this->regex_;
}

PossibleType RegexExpression::synthesizeType(
    const TypingContext & /*context*/) const
{
    return TypeClass{Type::RegularExpression};
}

QString RegexExpression::debug(const TypingContext & /*context*/) const
{
    return QString("RegEx(%1)").arg(this->regexString_);
}

QString RegexExpression::filterString() const
{
    auto s = this->regexString_;
    return QString("%1\"%2\"")
        .arg(this->caseInsensitive_ ? "ri" : "r")
        .arg(s.replace("\"", "\\\""));
}

}  // namespace chatterino::filters
