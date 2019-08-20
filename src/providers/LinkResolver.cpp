#include "providers/LinkResolver.hpp"

#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "messages/Link.hpp"
#include "singletons/Settings.hpp"

#include <QString>

namespace chatterino {

void LinkResolver::getLinkInfo(
    const QString url, std::function<void(QString, Link)> successCallback)
{
    if (!getSettings()->linkInfoTooltip)
    {
        successCallback("No link info loaded", Link(Link::Url, url));
        return;
    }
    // Uncomment to test crashes
    // QTimer::singleShot(3000, [=]() {
    NetworkRequest(Env::get().linkResolverUrl.arg(QString::fromUtf8(
                       QUrl::toPercentEncoding(url, "", "/:"))))
        .caller(QThread::currentThread())
        .timeout(30000)
        .onSuccess([successCallback, url](auto result) mutable -> Outcome {
            auto root = result.parseJson();
            auto statusCode = root.value("status").toInt();
            QString response = QString();
            QString linkString = url;
            if (statusCode == 200)
            {
                response = root.value("tooltip").toString();
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
                            Link(Link::Url, linkString));

            return Success;
        })
        .onError([successCallback, url](auto /*result*/) {
            successCallback("No link info found", Link(Link::Url, url));

            return true;
        })
        .execute();
    // });
}

}  // namespace chatterino
