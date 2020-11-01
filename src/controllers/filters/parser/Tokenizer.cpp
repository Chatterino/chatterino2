#include "controllers/filters/parser/Tokenizer.hpp"

namespace filterparser {

Tokenizer::Tokenizer(const QString &text)
{
    QRegularExpressionMatchIterator i = tokenRegex.globalMatch(text);
    while (i.hasNext())
    {
        auto text = i.next().captured();
        this->tokens_ << text;
        this->tokenTypes_ << this->tokenize(text);
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
        return this->tokens_.at(this->i_);
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
        qDebug() << "= current" << this->tokens_.at(this->i_ - 1);
        qDebug() << "= current type" << this->tokenTypes_.at(this->i_ - 1);
    }
    else
    {
        qDebug() << "= no current";
    }
    if (this->hasNext())
    {
        qDebug() << "= next" << this->tokens_.at(this->i_);
        qDebug() << "= next type" << this->tokenTypes_.at(this->i_);
    }
    else
    {
        qDebug() << "= no next";
    }
}

const QStringList Tokenizer::allTokens()
{
    return this->tokens_;
}

TokenType Tokenizer::tokenize(const QString &text)
{
    if (text == "&&")
        return TokenType::AND;
    else if (text == "||")
        return TokenType::OR;
    else if (text == "(")
        return TokenType::LP;
    else if (text == ")")
        return TokenType::RP;
    else if (text == "{")
        return TokenType::LIST_START;
    else if (text == "}")
        return TokenType::LIST_END;
    else if (text == ",")
        return TokenType::COMMA;
    else if (text == "+")
        return TokenType::PLUS;
    else if (text == "-")
        return TokenType::MINUS;
    else if (text == "*")
        return TokenType::MULTIPLY;
    else if (text == "/")
        return TokenType::DIVIDE;
    else if (text == "==")
        return TokenType::EQ;
    else if (text == "!=")
        return TokenType::NEQ;
    else if (text == "%")
        return TokenType::MOD;
    else if (text == "<")
        return TokenType::LT;
    else if (text == ">")
        return TokenType::GT;
    else if (text == "<=")
        return TokenType::LTE;
    else if (text == ">=")
        return TokenType::GTE;
    else if (text == "contains")
        return TokenType::CONTAINS;
    else if (text == "startswith")
        return TokenType::STARTS_WITH;
    else if (text == "endswith")
        return TokenType::ENDS_WITH;
    else if (text == "!")
        return TokenType::NOT;
    else
    {
        if (text.front() == '"' && text.back() == '"')
            return TokenType::STRING;

        if (validIdentifiersMap.keys().contains(text))
            return TokenType::IDENTIFIER;

        bool flag;
        if (text.toInt(&flag); flag)
            return TokenType::INT;
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

}  // namespace filterparser
