#include "ValueExpression.hpp"

#include "controllers/filters/lang/Tokenizer.hpp"

namespace chatterino::filters {

ValueExpression::ValueExpression(QVariant value, TokenType type)
    : value_(value)
    , type_(type)
{
}

QVariant ValueExpression::execute(const ContextMap &context) const
{
    if (this->type_ == TokenType::IDENTIFIER)
    {
        return context.value(this->value_.toString());
    }
    return this->value_;
}

PossibleType ValueExpression::synthesizeType() const
{
    switch (this->type_)
    {
        case TokenType::IDENTIFIER: {
            auto it = validIdentifiersMap.find(this->value_.toString());
            if (it != validIdentifiersMap.end())
            {
                return it.value().type;
            }

            return IllTyped{this, "Unable to synthesize identifier type"};
        }
        case TokenType::INT:
            return Type::Int;
        case TokenType::STRING:
            return Type::String;
        default:
            return IllTyped{this, "Invalid value type"};
    }
}

TokenType ValueExpression::type()
{
    return this->type_;
}

QString ValueExpression::debug() const
{
    return this->value_.toString();
}

QString ValueExpression::filterString() const
{
    switch (this->type_)
    {
        case INT:
            return QString::number(this->value_.toInt());
        case STRING:
            return QString("\"%1\"").arg(
                this->value_.toString().replace("\"", "\\\""));
        case IDENTIFIER:
            return this->value_.toString();
        default:
            return "";
    }
}

}  // namespace chatterino::filters
