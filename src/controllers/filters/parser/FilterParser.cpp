#include "FilterParser.hpp"

#include "Application.hpp"
#include "controllers/filters/parser/Types.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"

namespace filterparser {

ContextMap buildContextMap(const MessagePtr &m)
{
    auto watchingChannel =
        chatterino::getApp()->twitch.server->watchingChannel.get();

    /* Known Identifiers
     *
     * author.badges
     * author.color
     * author.name
     * author.no_color
     * author.subbed
     * author.sub_length
     *
     * channel.name
     * channel.watching
     *
     * flags.highlighted
     * flags.points_redeemed
     * flags.sub_message
     * flags.system_message
     * flags.whisper
     *
     * message.content
     * message.length
     *
     */

    using MessageFlag = chatterino::MessageFlag;

    QStringList badges;
    for (const auto &e : m->badges)
    {
        badges << e.key_;
    }

    bool watching = !watchingChannel->getName().isEmpty() &&
                    watchingChannel->getName().compare(
                        m->channelName, Qt::CaseInsensitive) == 0;

    bool subscribed = badges.contains("subscriber");
    int subLength = subscribed ? m->badgeInfos.at("subscriber").toInt() : 0;

    return {
        {"author.badges", badges},
        {"author.color", m->usernameColor},
        {"author.name", m->displayName},
        {"author.no_color", !m->usernameColor.isValid()},
        {"author.subbed", subscribed},
        {"author.sub_length", subLength},

        {"channel.name", m->channelName},
        {"channel.watching", watching},

        {"flags.highlighted", m->flags.has(MessageFlag::Highlighted)},
        {"flags.points_redeemed", m->flags.has(MessageFlag::RedeemedHighlight)},
        {"flags.sub_message", m->flags.has(MessageFlag::Subscription)},
        {"flags.system_message", m->flags.has(MessageFlag::System)},
        {"flags.whisper", m->flags.has(MessageFlag::Whisper)},

        {"message.content", m->messageText},
        {"message.length", m->messageText.length()},
    };
}

FilterParser::FilterParser(const QString &text)
    : text_(text)
    , tokenizer_(Tokenizer(text))
{
    this->builtExpression_ = this->parseExpression(true);
}

bool FilterParser::execute(const MessagePtr &message) const
{
    auto context = buildContextMap(message);
    return this->execute(context);
}

bool FilterParser::execute(const ContextMap &context) const
{
    return this->builtExpression_->execute(context).toBool();
}

bool FilterParser::valid() const
{
    return this->valid_;
}

Expression *FilterParser::parseExpression(bool top)
{
    Expression *e = this->parseAnd();
    while (this->tokenizer_.hasNext() &&
           this->tokenizer_.nextTokenType() == TokenType::OR)
    {
        this->tokenizer_.next();
        e = new BinaryOperation(TokenType::OR, e, this->parseAnd());
    }

    if (this->tokenizer_.hasNext() && top)
    {
        this->errorLog(QString("Unexpected token at end: %1")
                           .arg(this->tokenizer_.preview()));
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
        e = new BinaryOperation(TokenType::AND, e, this->parseUnary());
    }
    return e;
}

Expression *FilterParser::parseUnary()
{
    if (this->tokenizer_.hasNext() && this->tokenizer_.nextTokenIsUnaryOp())
    {
        auto nextType = this->tokenizer_.nextTokenType();
        this->tokenizer_.next();
        auto e = new UnaryOperation(nextType, this->parseCondition());
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
        if (this->tokenizer_.hasNext())
        {
            this->errorLog(QString("Missing closing parenthesis: got %1")
                               .arg(this->tokenizer_.preview()));
        }
        else
        {
            this->errorLog("Missing closing parenthesis at end of statement");
        }

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
            return new BinaryOperation(this->tokenizer_.tokenType(), value,
                                       this->parseValue());
        }
        else if (this->tokenizer_.nextTokenIsMathOp())
        {
            this->tokenizer_.next();
            value = new BinaryOperation(this->tokenizer_.tokenType(), value,
                                        this->parseValue());
        }
        else if (this->tokenizer_.nextTokenType() == TokenType::RP)
        {
            // RP, so move on
            break;
        }
        else if (!this->tokenizer_.nextTokenIsOp())
        {
            this->errorLog(QString("Expected an operator but got %1 %2")
                               .arg(this->tokenizer_.preview())
                               .arg(tokenTypeToInfoString(
                                   this->tokenizer_.nextTokenType())));
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
            this->tokenizer_.next();
            this->errorLog(QString("Expected value but got %1 %2")
                               .arg(this->tokenizer_.current())
                               .arg(tokenTypeToInfoString(type)));
        }
    }
    else
    {
        this->errorLog("Unexpected end of statement");
    }

    return new ValueExpression(0, TokenType::INT);
}

void FilterParser::errorLog(const QString &text, bool expand)
{
    this->valid_ = false;
    if (expand || this->parseLog_.size() == 0)
    {
        this->parseLog_ << text;
    }
}

const QStringList &FilterParser::errors() const
{
    return this->parseLog_;
}

const QString FilterParser::debugString() const
{
    return this->builtExpression_->debug();
}

}  // namespace filterparser
