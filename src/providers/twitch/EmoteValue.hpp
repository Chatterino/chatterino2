#pragma once

#include <QString>

namespace chatterino {

struct EmoteValue {
public:
    int getSet()
    {
        return this->set_;
    }

    int getId()
    {
        return this->id_;
    }

    const QString &getChannelName()
    {
        return this->channelName_;
    }

private:
    int set_;
    int id_;
    QString channelName_;
};

}  // namespace chatterino
