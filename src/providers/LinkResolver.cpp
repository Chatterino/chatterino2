#include "providers/LinkResolver.hpp"

#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "messages/Link.hpp"
#include "singletons/Settings.hpp"
#include "messages/Image.hpp"

#include <QString>

namespace chatterino {

void LinkResolver::getLinkInfo(
    const QString url, QObject *caller,
    std::function<void(QString, Link, ImagePtr)> successCallback)
{
    if (!getSettings()->linkInfoTooltip)
    {
        successCallback("No link info loaded", Link(Link::Url, url), nullptr);
        return;
    }
    // Uncomment to test crashes
    // QTimer::singleShot(3000, [=]() {
    NetworkRequest(Env::get().linkResolverUrl.arg(QString::fromUtf8(
                       QUrl::toPercentEncoding(url, "", "/:"))))
        .caller(caller)
        .timeout(30000)
        .onSuccess([successCallback, url](auto result) mutable -> Outcome {
            auto root = result.parseJson();
            auto statusCode = root.value("status").toInt();
            QString response = QString();
            QString linkString = url;
            ImagePtr thumbnail = nullptr;
            if (statusCode == 200)
            {
                response = root.value("tooltip").toString();
                thumbnail = Image::fromUrl({root.value("thumbnail").toString()});
                if (getSettings()->unshortLinks)
                {
                    linkString = root.value("link").toString();
                }
            }
            else
            {
                response = root.value("message").toString();
            }
            successCallback(QUrl::fromPercentEncoding(response.toUtf8()),
                            Link(Link::Url, linkString),
                            thumbnail);

            return Success;
        })
        .onError([successCallback, url](auto /*result*/) {
            successCallback("No link info found", Link(Link::Url, url), nullptr);
        })
        .execute();
    // });
}

}  // namespace chatterino
