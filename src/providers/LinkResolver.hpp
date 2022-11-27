#pragma once

#include "messages/Image.hpp"
#include "messages/Link.hpp"

#include <QString>

#include <functional>

namespace chatterino {

class LinkResolver
{
public:
    static void getLinkInfo(
        const QString url, QObject *caller,
        std::function<void(QString, Link, ImagePtr)> callback);
};

}  // namespace chatterino
