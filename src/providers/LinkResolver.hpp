#pragma once

#include <QString>
#include <functional>

#include "messages/Link.hpp"

namespace chatterino {

class LinkResolver
{
public:
    static void getLinkInfo(const QString url,
                            std::function<void(QString, Link)> callback);

private:
};

}  // namespace chatterino
