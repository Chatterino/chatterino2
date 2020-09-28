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
    int subLength = (subscribed && m->badgeInfos.count("subscriber") != 0)
                        ? m->badgeInfos.at("subscriber").toInt()
                        : 0;

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
    , builtExpression_(this->parseExpression(true))
{
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

ExpressionPtr FilterParser::parseExpression(bool top)
{
    auto e = this->parseAnd();
    while (this->tokenizer_.hasNext() &&
           this->tokenizer_.nextTokenType() == TokenType::OR)
    {
        this->tokenizer_.next();
        auto nextAnd = this->parseAnd();
        e = std::make_unique<BinaryOperation>(TokenType::OR, std::move(e),
                                              std::move(nextAnd));
    }

    if (this->tokenizer_.hasNext() && top)
    {
        this->errorLog(QString("Unexpected token at end: %1")
                           .arg(this->tokenizer_.preview()));
    }

    return e;
}

ExpressionPtr FilterParser::parseAnd()
{
    auto e = this->parseUnary();
    while (this->tokenizer_.hasNext() &&
           this->tokenizer_.nextTokenType() == TokenType::AND)
    {
        this->tokenizer_.next();
        auto nextUnary = this->parseUnary();
        e = std::make_unique<BinaryOperation>(TokenType::AND, std::move(e),
                                              std::move(nextUnary));
    }
    return e;
}

ExpressionPtr FilterParser::parseUnary()
{
    if (this->tokenizer_.hasNext() && this->tokenizer_.nextTokenIsUnaryOp())
    {
        this->tokenizer_.next();
        auto type = this->tokenizer_.tokenType();
        auto nextCondition = this->parseCondition();
        return std::make_unique<UnaryOperation>(type, std::move(nextCondition));
    }
    else
    {
        return this->parseCondition();
    }
}

ExpressionPtr FilterParser::parseParentheses()
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
            this->errorLog(QString("Missing closing parentheses: got %1")
                               .arg(this->tokenizer_.preview()));
        }
        else
        {
            this->errorLog("Missing closing parentheses at end of statement");
        }

        return e;
    }
}

ExpressionPtr FilterParser::parseCondition()
{
    ExpressionPtr value;
    // parse expression wrapped in parentheses
    if (this->tokenizer_.hasNext() &&
        this->tokenizer_.nextTokenType() == TokenType::LP)
    {
        // get value inside parentheses
        value = this->parseParentheses();
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
            auto type = this->tokenizer_.tokenType();
            auto nextValue = this->parseValue();
            return std::make_unique<BinaryOperation>(type, std::move(value),
                                                     std::move(nextValue));
        }
        else if (this->tokenizer_.nextTokenIsMathOp())
        {
            this->tokenizer_.next();
            auto type = this->tokenizer_.tokenType();
            auto nextValue = this->parseValue();
            value = std::make_unique<BinaryOperation>(type, std::move(value),
                                                      std::move(nextValue));
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

ExpressionPtr FilterParser::parseValue()
{
    // parse a literal or an expression wrapped in parenthsis
    if (this->tokenizer_.hasNext())
    {
        auto type = this->tokenizer_.nextTokenType();
        if (type == TokenType::INT)
        {
            return std::make_unique<ValueExpression>(
                this->tokenizer_.next().toInt(), type);
        }
        else if (type == TokenType::STRING)
        {
            auto before = this->tokenizer_.next();
            // remove quote marks
            auto val = before.mid(1);
            val.chop(1);
            val = val.replace("\\\"", "\"");
            return std::make_unique<ValueExpression>(val, type);
        }
        else if (type == TokenType::IDENTIFIER)
        {
            return std::make_unique<ValueExpression>(this->tokenizer_.next(),
                                                     type);
        }
        else if (type == TokenType::LP)
        {
            return this->parseParentheses();
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

    return std::make_unique<ValueExpression>(0, TokenType::INT);
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

const QString FilterParser::filterString() const
{
    return this->builtExpression_->filterString();
}

}  // namespace filterparser
