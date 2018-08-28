#pragma once

#include <QString>
#include <functional>

namespace chatterino {

class TwitchApi
{
public:
    static void findUserId(const QString user,
                           std::function<void(QString)> callback);

private:
};

}  // namespace chatterino
