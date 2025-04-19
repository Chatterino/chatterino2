#pragma once

#include <QString>

namespace chatterino::eventsub::lib {

class Logger
{
public:
    virtual ~Logger() = default;
    virtual void debug(const QString &msg) = 0;
    virtual void warn(const QString &msg) = 0;
};

class NullLogger : public Logger
{
public:
    void debug(const QString &msg) override
    {
    }

    void warn(const QString &msg) override
    {
    }
};

}  // namespace chatterino::eventsub::lib
