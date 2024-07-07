#pragma once

#include "singletons/Logging.hpp"

#include <gmock/gmock.h>
#include <QString>
#include <QStringList>

namespace chatterino::mock {

class Logging : public ILogging
{
public:
    Logging() = default;
    ~Logging() override = default;

    MOCK_METHOD(void, addMessage,
                (const QString &channelName, MessagePtr message,
                 const QString &platformName,
                 std::optional<MessageFlags> overridingFlags,
                 MessageContext context),
                (override));
};

class EmptyLogging : public ILogging
{
public:
    EmptyLogging() = default;
    ~EmptyLogging() override = default;

    void addMessage(const QString &channelName, MessagePtr message,
                    const QString &platformName,
                    std::optional<MessageFlags> overridingFlags,
                    MessageContext context) override
    {
        //
    }
};

}  // namespace chatterino::mock
