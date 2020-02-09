#pragma once

#include <QString>
#include <functional>

namespace chatterino {

class TwitchApi
{
public:
    static void findUserName(const QString userid,
                             std::function<void(QString)> callback);

private:
};

}  // namespace chatterino
