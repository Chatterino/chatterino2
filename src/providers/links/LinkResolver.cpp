#include "providers/links/LinkResolver.hpp"

#include "common/Env.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "providers/links/LinkInfo.hpp"
#include "singletons/Settings.hpp"

#include <QStringBuilder>

namespace chatterino {

void LinkResolver::resolve(LinkInfo *info)
{
    using State = LinkInfo::State;

    assert(info);

    if (info->state() != State::Created)
    {
        // The link is already resolved or is currently loading
        return;
    }

    if (!getSettings()->linkInfoTooltip)
    {
        return;
    }

    info->setTooltip("Loading...");
    info->setState(State::Loading);

    NetworkRequest(Env::get().linkResolverUrl.arg(QString::fromUtf8(
                       QUrl::toPercentEncoding(info->originalUrl(), {}, "/:"))))
        .caller(info)
        .timeout(30000)
        .onSuccess([info](const NetworkResult &result) {
            const auto root = result.parseJson();
            QString response;
            QString url;
            ImagePtr thumbnail = nullptr;
            if (root["status"].toInt() == 200)
            {
                response = root["tooltip"].toString();

                if (root.contains("thumbnail"))
                {
                    info->setThumbnail(
                        Image::fromUrl({root["thumbnail"].toString()}));
                }
                if (getSettings()->unshortLinks && root.contains("link"))
                {
                    info->setResolvedUrl(root["link"].toString());
                }
            }
            else
            {
                response = root["message"].toString();
            }

            info->setTooltip(QUrl::fromPercentEncoding(response.toUtf8()));
            info->setState(State::Resolved);
        })
        .onError([info](const auto &result) {
            info->setTooltip(u"No link info found (" % result.formatError() %
                             u')');
            info->setState(State::Errored);
        })
        .execute();
}

}  // namespace chatterino
