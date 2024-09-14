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
                 const QString &platformName, const QString &streamID),
                (override));

    MOCK_METHOD(void, closeChannel,
                (const QString &channelName, const QString &platformName),
                (override));
};

class EmptyLogging : public ILogging
{
public:
    EmptyLogging() = default;
    ~EmptyLogging() override = default;

    void addMessage(const QString &channelName, MessagePtr message,
                    const QString &platformName,
                    const QString &streamID) override
    {
        //
    }

    void closeChannel(const QString &channelName,
                      const QString &platformName) override
    {
        //
    }
};

}  // namespace chatterino::mock
