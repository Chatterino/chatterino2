#pragma once

#include "common/Singleton.hpp"

#include <QString>

namespace chatterino {

class TwitchApi
{
public:
    static void FindUserId(const QString user, std::function<void(QString)> callback);

private:
};

}  // namespace chatterino
