#include "providers/LinkResolver.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"

#include <QString>

namespace chatterino {

void LinkResolver::getLinkInfo(const QString url,
                           std::function<void(QString)> successCallback)
{
    QString requestUrl("https://braize.pajlada.com/chatterino/link_resolver/" + 
        QUrl::toPercentEncoding(url, "", "/:"));

    NetworkRequest request(requestUrl);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([successCallback](auto result) mutable -> Outcome {
        auto root = result.parseJson();
        auto statusCode = root.value("status").toInt();
        QString response = QString();
        if (statusCode == 200) {
            response = root.value("tooltip").toString();
        } else {
            response = root.value("message").toString();
        }
        successCallback(QUrl::fromPercentEncoding(response.toUtf8()));

        return Success;
    });

    request.onError([successCallback](auto result) {
        successCallback("No link info found");

        return true;
    });

    request.execute();
}

}  // namespace chatterino
