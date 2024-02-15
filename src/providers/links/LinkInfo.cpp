#include "providers/links/LinkInfo.hpp"

#include "common/Env.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "singletons/Settings.hpp"

#include <QStringBuilder>

namespace chatterino {

LinkInfo::LinkInfo(QString url)
    : QObject(nullptr)
    , url_(std::move(url))
    , tooltip_(this->url_)
{
}

LinkInfo::~LinkInfo() = default;

QString LinkInfo::url() const
{
    return this->url_;
}

bool LinkInfo::isResolved() const
{
    return this->lifecycle_ == Lifecycle::Resolved;
}

bool LinkInfo::isLoading() const
{
    return this->lifecycle_ == Lifecycle::Loading;
}

bool LinkInfo::hasError() const
{
    return this->lifecycle_ == Lifecycle::Errored;
}

bool LinkInfo::hasThumbnail() const
{
    return this->thumbnail_ && !this->thumbnail_->url().string.isEmpty();
}

QString LinkInfo::tooltip() const
{
    return this->tooltip_;
}

ImagePtr LinkInfo::thumbnail() const
{
    return this->thumbnail_;
}

void LinkInfo::setLifecycle(Lifecycle lifecycle)
{
    assertInGuiThread();

    if (this->lifecycle_ == lifecycle)
    {
        return;
    }

    this->lifecycle_ = lifecycle;
    this->lifecycleChanged();
}

void LinkInfo::ensureLoadingStarted()
{
    if (this->lifecycle_ != Lifecycle::Created)
    {
        return;
    }

    if (!getSettings()->linkInfoTooltip)
    {
        return;
    }

    this->setLifecycle(Lifecycle::Loading);

    NetworkRequest(Env::get().linkResolverUrl.arg(QString::fromUtf8(
                       QUrl::toPercentEncoding(this->url_, {}, "/:"))))
        .caller(this)
        .timeout(30000)
        .onSuccess([this](const NetworkResult &result) {
            const auto root = result.parseJson();
            QString response;
            QString url;
            ImagePtr thumbnail = nullptr;
            if (root["status"].toInt() == 200)
            {
                response = root["tooltip"].toString();

                if (root.contains("thumbnail"))
                {
                    this->thumbnail_ =
                        Image::fromUrl({root["thumbnail"].toString()});
                }
                if (getSettings()->unshortLinks && root.contains("link"))
                {
                    this->url_ = root["link"].toString();
                }
            }
            else
            {
                response = root["message"].toString();
            }

            this->tooltip_ = QUrl::fromPercentEncoding(response.toUtf8());
            this->setLifecycle(Lifecycle::Resolved);
        })
        .onError([this](const auto &result) {
            this->tooltip_ =
                u"No link info found (" % result.formatError() % u')';
            this->setLifecycle(Lifecycle::Errored);
        })
        .execute();
}

}  // namespace chatterino
