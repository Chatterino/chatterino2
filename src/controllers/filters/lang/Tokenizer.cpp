#include "controllers/filters/lang/Tokenizer.hpp"

#include "common/QLogging.hpp"

namespace chatterino::filters {

QString tokenTypeToInfoString(TokenType type)
{
    switch (type)
    {
        case AND:
            return "And";
        case OR:
            return "Or";
        case LP:
            return "<left parenthesis>";
        case RP:
            return "<right parenthesis>";
        case LIST_START:
            return "<list start>";
        case LIST_END:
            return "<list end>";
        case COMMA:
            return "<comma>";
        case PLUS:
            return "Plus";
        case MINUS:
            return "Minus";
        case MULTIPLY:
            return "Multiply";
        case DIVIDE:
            return "Divide";
        case MOD:
            return "Mod";
        case EQ:
            return "Eq";
        case NEQ:
            return "NotEq";
        case LT:
            return "LessThan";
        case GT:
            return "GreaterThan";
        case LTE:
            return "LessThanEq";
        case GTE:
            return "GreaterThanEq";
        case CONTAINS:
            return "Contains";
        case STARTS_WITH:
            return "StartsWith";
        case ENDS_WITH:
            return "EndsWith";
        case MATCH:
            return "Match";
        case NOT:
            return "Not";
        case STRING:
            return "<string>";
        case INT:
            return "<int>";
        case IDENTIFIER:
            return "<identifier>";
        case CONTROL_START:
        case CONTROL_END:
        case BINARY_START:
        case BINARY_END:
        case UNARY_START:
        case UNARY_END:
        case MATH_START:
        case MATH_END:
        case OTHER_START:
        case NONE:
        default:
            return "<unknown>";
    }
}

Tokenizer::Tokenizer(const QString &text)
{
    QRegularExpressionMatchIterator i = tokenRegex.globalMatch(text);
    while (i.hasNext())
    {
        auto capturedText = i.next().captured();
        this->tokens_ << capturedText;
        this->tokenTypes_ << this->tokenize(capturedText);
    }
}

bool Tokenizer::hasNext() const
{
    return this->i_ < this->tokens_.length();
}

QString Tokenizer::next()
{
    this->i_++;
    return this->tokens_.at(this->i_ - 1);
}

QString Tokenizer::current() const
{
    return this->tokens_.at(this->i_ - 1);
}

QString Tokenizer::preview() const
{
    if (this->hasNext())
    {
        return this->tokens_.at(this->i_);
    }
    return "";
}

TokenType Tokenizer::nextTokenType() const
{
    return this->tokenTypes_.at(this->i_);
}

TokenType Tokenizer::tokenType() const
{
    return this->tokenTypes_.at(this->i_ - 1);
}

bool Tokenizer::nextTokenIsOp() const
{
    return this->typeIsOp(this->nextTokenType());
}

bool Tokenizer::nextTokenIsBinaryOp() const
{
    return this->typeIsBinaryOp(this->nextTokenType());
}

bool Tokenizer::nextTokenIsUnaryOp() const
{
    return this->typeIsUnaryOp(this->nextTokenType());
}

bool Tokenizer::nextTokenIsMathOp() const
{
    return this->typeIsMathOp(this->nextTokenType());
}

void Tokenizer::debug()
{
    if (this->i_ > 0)
    {
        qCDebug(chatterinoTokenizer)
            << "= current" << this->tokens_.at(this->i_ - 1);
        qCDebug(chatterinoTokenizer)
            << "= current type" << this->tokenTypes_.at(this->i_ - 1);
    }
    else
    {
        qCDebug(chatterinoTokenizer) << "= no current";
    }
    if (this->hasNext())
    {
        qCDebug(chatterinoTokenizer) << "= next" << this->tokens_.at(this->i_);
        qCDebug(chatterinoTokenizer)
            << "= next type" << this->tokenTypes_.at(this->i_);
    }
    else
    {
        qCDebug(chatterinoTokenizer) << "= no next";
    }
}

const QStringList Tokenizer::allTokens()
{
    return this->tokens_;
}

TokenType Tokenizer::tokenize(const QString &text)
{
    if (text == "&&")
    {
        return TokenType::AND;
    }
    else if (text == "||")
    {
        return TokenType::OR;
    }
    else if (text == "(")
    {
        return TokenType::LP;
    }
    else if (text == ")")
    {
        return TokenType::RP;
    }
    else if (text == "{")
    {
        return TokenType::LIST_START;
    }
    else if (text == "}")
    {
        return TokenType::LIST_END;
    }
    else if (text == ",")
    {
        return TokenType::COMMA;
    }
    else if (text == "+")
    {
        return TokenType::PLUS;
    }
    else if (text == "-")
    {
        return TokenType::MINUS;
    }
    else if (text == "*")
    {
        return TokenType::MULTIPLY;
    }
    else if (text == "/")
    {
        return TokenType::DIVIDE;
    }
    else if (text == "==")
    {
        return TokenType::EQ;
    }
    else if (text == "!=")
    {
        return TokenType::NEQ;
    }
    else if (text == "%")
    {
        return TokenType::MOD;
    }
    else if (text == "<")
    {
        return TokenType::LT;
    }
    else if (text == ">")
    {
        return TokenType::GT;
    }
    else if (text == "<=")
    {
        return TokenType::LTE;
    }
    else if (text == ">=")
    {
        return TokenType::GTE;
    }
    else if (text == "contains")
    {
        return TokenType::CONTAINS;
    }
    else if (text == "startswith")
    {
        return TokenType::STARTS_WITH;
    }
    else if (text == "endswith")
    {
        return TokenType::ENDS_WITH;
    }
    else if (text == "match")
    {
        return TokenType::MATCH;
    }
    else if (text == "!")
    {
        return TokenType::NOT;
    }
    else
    {
        if ((text.startsWith("r\"") || text.startsWith("ri\"")) &&
            text.back() == '"')
        {
            return TokenType::REGULAR_EXPRESSION;
        }

        if (text.front() == '"' && text.back() == '"')
        {
            return TokenType::STRING;
        }

        if (validIdentifiersMap.keys().contains(text))
        {
            return TokenType::IDENTIFIER;
        }

        bool flag;
        if (text.toInt(&flag); flag)
        {
            return TokenType::INT;
        }
    }

    return TokenType::NONE;
}

bool Tokenizer::typeIsOp(TokenType token)
{
    return typeIsBinaryOp(token) || typeIsUnaryOp(token) ||
           typeIsMathOp(token) || token == TokenType::AND ||
           token == TokenType::OR;
}

bool Tokenizer::typeIsBinaryOp(TokenType token)
{
    return token > TokenType::BINARY_START && token < TokenType::BINARY_END;
}

bool Tokenizer::typeIsUnaryOp(TokenType token)
{
    return token > TokenType::UNARY_START && token < TokenType::UNARY_END;
}

bool Tokenizer::typeIsMathOp(TokenType token)
{
    return token > TokenType::MATH_START && token < TokenType::MATH_END;
}

}  // namespace chatterino::filters
