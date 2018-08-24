#include "providers/bttv/BttvTooltip.hpp"

#include "common/Common.hpp"
#include "common/NetworkRequest.hpp"

#include <QString>

namespace chatterino {

void BttvTooltip::getUrlTooltip(const QString url,
                           std::function<void(QString)> successCallback)
{
    QString requestUrl("https://api.betterttv.net/2/link_resolver/" + 
        QUrl::toPercentEncoding(url, "", "/:"));

    NetworkRequest request(requestUrl);
    request.setCaller(QThread::currentThread());
    request.setTimeout(30000);
    request.onSuccess([successCallback](auto result) mutable -> Outcome {
        auto root = result.parseJson();
        /* When tooltip is not a string, in this case, 
        onError runs before onSuccess, 
        so there is no point in doing "if" condition. */
        auto tooltip = root.value("tooltip").toString();
        successCallback(QUrl::fromPercentEncoding(tooltip.toUtf8()));

        return Success;
    });

    request.onError([successCallback](auto result) {
        successCallback("No link info found");

        return true;
    });

    request.execute();
}

}  // namespace chatterino
