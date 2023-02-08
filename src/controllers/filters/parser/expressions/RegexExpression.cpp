#include "RegexExpression.hpp"

namespace filterparser {

RegexExpression::RegexExpression(QString regex, bool caseInsensitive)
    : regexString_(regex)
    , caseInsensitive_(caseInsensitive)
    , regex_(QRegularExpression(
          regex, caseInsensitive ? QRegularExpression::CaseInsensitiveOption
                                 : QRegularExpression::NoPatternOption)){};

QVariant RegexExpression::execute(const ContextMap &) const
{
    return this->regex_;
}

PossibleType RegexExpression::synthesizeType() const
{
    return Type::RegularExpression;
}

QString RegexExpression::debug() const
{
    return this->regexString_;
}

QString RegexExpression::filterString() const
{
    auto s = this->regexString_;
    return QString("%1\"%2\"")
        .arg(this->caseInsensitive_ ? "ri" : "r")
        .arg(s.replace("\"", "\\\""));
}

}  // namespace filterparser
