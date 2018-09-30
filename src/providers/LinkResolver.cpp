#include "providers/LinkResolver.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"
#include "messages/Link.hpp"
#include "singletons/Settings.hpp"

#include <QString>

namespace chatterino {

void LinkResolver::getLinkInfo(
    const QString url, std::function<void(QString, Link)> successCallback)
{
    QString requestUrl("https://braize.pajlada.com/chatterino/link_resolver/" +
                       QUrl::toPercentEncoding(url, "", "/:"));

    NetworkRequest request(requestUrl);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([successCallback, url](auto result) mutable -> Outcome {
        auto root = result.parseJson();
        auto statusCode = root.value("status").toInt();
        QString response = QString();
        QString linkString = url;
        if (statusCode == 200) {
            response = root.value("tooltip").toString();
            if (getSettings()->enableUnshortLinks) {
                linkString = root.value("link").toString();
            }
        } else {
            response = root.value("message").toString();
        }
        successCallback(QUrl::fromPercentEncoding(response.toUtf8()),
                        Link(Link::Url, linkString));

        return Success;
    });

    request.onError([successCallback, url](auto result) {
        successCallback("No link info found", Link(Link::Url, url));

        return true;
    });

    request.execute();
}

}  // namespace chatterino
