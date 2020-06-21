#include "FilterParser.hpp"

#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

namespace {

    ContextMap buildContextMap(const MessagePtr &m)
    {
        // Known identifiers:
        // message.content
        // message.length
        // author.name
        // author.subscribed
        // author.subscription_length
        // author.message_color
        // author.badges

        QStringList badges;
        for (const auto &e : m->badges)
        {
            badges << QString("%1").arg(e.key_);
        }

        bool subscribed = badges.contains("subscriber");
        int subLength = subscribed ? m->badgeInfos.at("subscriber").toInt() : 0;

        return {{"message.content", m->messageText},
                {"message.length", m->messageText.length()},
                {"author.name", m->displayName},
                {"author.subscribed", subscribed},
                {"author.subscription_length", subLength},
                {"author.message_color", QColor("#FF0000")},
                {"author.badges", badges}};
    }
}  // namespace

FilterParser::FilterParser(const QString &text)
    : text_(text)
    , tokenizer_(Tokenizer(text))
{
    this->builtExpression_ = this->parseExpression();
    qDebug() << "Fully-built expression:" << this->builtExpression_->debug();
}

bool FilterParser::execute(const MessagePtr &message) const
{
    auto map = buildContextMap(message);
    return this->builtExpression_->execute(map).toBool();
}

Expression *FilterParser::parseExpression()
{
    Expression *e = this->parseAnd();
    while (this->tokenizer_.hasNext() &&
           this->tokenizer_.nextTokenType() == TokenType::OR)
    {
        this->tokenizer_.next();
        e = new BinaryOperation(BinaryOperator::Or, e, this->parseAnd());
    }
    return e;
}

Expression *FilterParser::parseAnd()
{
    auto e = this->parseUnary();
    while (this->tokenizer_.hasNext() &&
           this->tokenizer_.nextTokenType() == TokenType::AND)
    {
        this->tokenizer_.next();
        e = new BinaryOperation(BinaryOperator::And, e, this->parseUnary());
    }
    return e;
}

Expression *FilterParser::parseUnary()
{
    if (this->tokenizer_.hasNext() && this->tokenizer_.nextTokenIsUnaryOp())
    {
        auto nextType = this->tokenizer_.nextTokenType();
        this->tokenizer_.next();
        auto e =
            new UnaryOperation(UnaryOperator(nextType), this->parseCondition());
        return e;
    }
    else
    {
        return this->parseCondition();
    }
}

Expression *FilterParser::parseCondition()
{
    if (this->tokenizer_.hasNext() &&
        this->tokenizer_.nextTokenType() == TokenType::LP)
    {
        this->tokenizer_.next();
        auto e = this->parseExpression();
        if (this->tokenizer_.hasNext() &&
            this->tokenizer_.nextTokenType() == TokenType::RP)
        {
            this->tokenizer_.next();
            return e;
        }
        else
        {
            qDebug() << "missing closing parenthesis";
            return e;
        }
    }

    auto value = this->parseValue();
    if (this->tokenizer_.hasNext())
    {
        if (this->tokenizer_.nextTokenIsBinaryOp())
        {
            this->tokenizer_.next();
            return new BinaryOperation(
                BinaryOperator(this->tokenizer_.tokenType()), value,
                this->parseValue());
        }
        else
        {
            qDebug() << "got unexpected token:"
                     << this->tokenizer_.nextTokenType();
        }
    }
    else
    {
        qDebug() << "ran out of tokens";
    }
    qDebug() << "expected an operator!";
    //return new ValueExpression(0, TokenType::INT);
    return value;
}

Expression *FilterParser::parseValue()
{
    if (this->tokenizer_.hasNext())
    {
        auto type = this->tokenizer_.nextTokenType();
        if (type == TokenType::INT)
        {
            return new ValueExpression(this->tokenizer_.next().toInt(), type);
        }
        else if (type == TokenType::STRING)
        {
            auto before = this->tokenizer_.next();
            auto val = before.mid(1);
            val.chop(1);
            return new ValueExpression(val, type);
        }
        else if (type == TokenType::IDENTIFIER)
        {
            return new ValueExpression(this->tokenizer_.next(), type);
        }
        else if (type == TokenType::LP)
        {
            this->tokenizer_.next();
            return this->parseAnd();
        }
        else
        {
            qDebug() << "unexpected value type:" << int(type);
        }
    }
    else
    {
        qDebug() << "ran out of tokens";
    }

    qDebug() << "expected a value!";
    return new ValueExpression(0, TokenType::INT);
}

}  // namespace filterparser
