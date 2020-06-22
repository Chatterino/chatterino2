#include "FilterParser.hpp"

#include "controllers/filters/parser/Types.hpp"

namespace filterparser {

namespace {

    ContextMap buildContextMap(const MessagePtr &m)
    {
        // Known identifiers:
        // message.content
        // message.length
        // message.highlighted
        // author.name
        // author.subscribed
        // author.subscription_length
        // author.color
        // author.no_color
        // author.badges
        // channel.name

        QStringList badges;
        for (const auto &e : m->badges)
        {
            badges << e.key_;
        }

        bool subscribed = badges.contains("subscriber");
        int subLength = subscribed ? m->badgeInfos.at("subscriber").toInt() : 0;

        return {{"message.content", m->messageText},
                {"message.length", m->messageText.length()},
                {"message.highlighted",
                 m->flags.has(chatterino::MessageFlag::Highlighted)},
                {"author.name", m->displayName},
                {"author.subscribed", subscribed},
                {"author.subscription_length", subLength},
                {"author.color", m->usernameColor},
                {"author.no_color", !m->usernameColor.isValid()},
                {"author.badges", badges},
                {"channel.name", m->channelName}};
    }
}  // namespace

FilterParser::FilterParser(const QString &text)
    : text_(text)
    , tokenizer_(Tokenizer(text))
{
    qDebug() << "---- parsing ----";
    this->builtExpression_ = this->parseExpression();
    qDebug().noquote() << "Fully-built expression:\n" +
                              this->builtExpression_->debug() + "\n";
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

Expression *FilterParser::parseParenthesis()
{
    // Don't call .next() before calling this method
    assert(this->tokenizer_.nextTokenType() == TokenType::LP);

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
        this->tokenizer_.debug();
        return e;
    }
}

Expression *FilterParser::parseCondition()
{
    Expression *value = nullptr;
    // parse expression wrapped in parenthesis
    if (this->tokenizer_.hasNext() &&
        this->tokenizer_.nextTokenType() == TokenType::LP)
    {
        // get value inside parenthesis
        value = this->parseParenthesis();
    }
    else
    {
        // get current value
        value = this->parseValue();
    }

    // expecting an operator or nothing
    while (this->tokenizer_.hasNext())
    {
        if (this->tokenizer_.nextTokenIsBinaryOp())
        {
            this->tokenizer_.next();
            return new BinaryOperation(
                BinaryOperator(this->tokenizer_.tokenType()), value,
                this->parseValue());
        }
        else if (this->tokenizer_.nextTokenIsMathOp())
        {
            this->tokenizer_.next();
            value = new BinaryOperation(
                BinaryOperator(this->tokenizer_.tokenType()), value,
                this->parseValue());
        }
        else if (this->tokenizer_.nextTokenType() != TokenType::RP)
        {
            qDebug() << "expected operator but got " +
                            QString::number(this->tokenizer_.nextTokenType());
            this->tokenizer_.debug();
            break;
        }
        else
        {
            break;
        }
    }

    return value;
}

Expression *FilterParser::parseValue()
{
    // parse a literal or an expression wrapped in parenthsis
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
            // remove quote marks
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
            return this->parseParenthesis();
        }
        else
        {
            qDebug() << "expected value but got " + QString::number(type);
            this->tokenizer_.debug();
        }
    }
    else
    {
        qDebug() << "unexpected end of statement. expected value";
        this->tokenizer_.debug();
    }

    return new ValueExpression(0, TokenType::INT);
}

}  // namespace filterparser
