// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/MessageSimilarity.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "messages/Message.hpp"
#include "mocks/BaseApplication.hpp"
#include "Test.hpp"

#include <QDateTime>

#include <memory>
#include <vector>

namespace chatterino {

namespace {

class MockApplication : public mock::BaseApplication
{
public:
    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    AccountController accounts;
};

MessagePtr makeMessage(const QDateTime &time)
{
    auto message = std::make_shared<Message>();
    message->loginName = "forsen";
    message->messageText = "i'm chicken";
    message->serverReceivedTime = time;
    return message;
}

}  // namespace

TEST(MessageSimilarity, R9KUsesMessageTimestamps)
{
    MockApplication app;
    app.settings.similarityEnabled = true;
    app.settings.hideSimilarMyself = true;
    app.settings.hideSimilarBySameUser = true;
    app.settings.hideSimilarMaxDelay = 5;
    app.settings.hideSimilarMaxMessagesToCheck = 10;
    app.settings.similarityPercentage = 0.9F;

    const auto time =
        QDateTime::fromString("1984-04-20T23:59:58Z", Qt::ISODate);
    auto previous = makeMessage(time);
    std::vector<MessagePtr> messages{previous};

    auto withinDelay = makeMessage(time.addSecs(4));
    setSimilarityFlags(withinDelay, messages);
    EXPECT_TRUE(withinDelay->flags.has(MessageFlag::Similar));

    auto beyondDelay = makeMessage(time.addSecs(6));
    setSimilarityFlags(beyondDelay, messages);
    EXPECT_FALSE(beyondDelay->flags.has(MessageFlag::Similar));
}

}  // namespace chatterino
