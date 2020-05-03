#pragma once

#include <QString>
#include <functional>

#include "messages/Image.hpp"
#include "messages/Link.hpp"

namespace chatterino {

class LinkResolver
{
public:
    static void getLinkInfo(
        const QString url, QObject *caller,
        std::function<void(QString, Link, ImagePtr)> callback);
};

}  // namespace chatterino
