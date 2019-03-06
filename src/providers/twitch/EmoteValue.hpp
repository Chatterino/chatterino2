#pragma once

#include <QString>

namespace chatterino
{
    struct EmoteValue
    {
    public:
        int getSet()
        {
            return set_;
        }

        int getId()
        {
            return id_;
        }

        const QString& getChannelName()
        {
            return channelName_;
        }

    private:
        int set_;
        int id_;
        QString channelName_;
    };

}  // namespace chatterino
