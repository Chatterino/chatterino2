#pragma once

#include <QObject>
#include <QString>

#include <functional>
#include <memory>

namespace chatterino {

class Image;
struct Link;
using ImagePtr = std::shared_ptr<Image>;

class LinkResolver
{
public:
    static void getLinkInfo(
        const QString url, QObject *caller,
        std::function<void(QString, Link, ImagePtr)> callback);
};

}  // namespace chatterino
