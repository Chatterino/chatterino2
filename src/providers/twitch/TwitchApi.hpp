#pragma once

#include <QString>

namespace chatterino {

class TwitchApi
{
public:
    static void findUserId(const QString user, std::function<void(QString)> callback);

private:
};

}  // namespace chatterino
