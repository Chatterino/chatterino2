#pragma once

#include "common/NetworkRequest.hpp"

#include <QJsonObject>
#include <QString>
#include <functional>
#include <vector>

namespace chatterino {

struct GitHubReleaseAsset {
    QString name;
    QString downloadURL;

    GitHubReleaseAsset()
        : name("")
        , downloadURL("")
    {
    }

    GitHubReleaseAsset(QJsonObject root)
        : name(root.value("name").toString())
        , downloadURL(root.value("browser_download_url").toString())
    {
    }
};

struct GitHubRelease {
    QString name;
    QString tagName;
    QString htmlURL;
    bool prerelease;
    std::vector<GitHubReleaseAsset> assets;

    GitHubRelease(QJsonObject root)
        : name(root.value("name").toString())
        , tagName(root.value("tag_name").toString())
        , htmlURL(root.value("html_url").toString())
        , prerelease(root.value("prerelease").toBool())
    {
        const auto assetArray = root.value("assets").toArray();
        this->assets.reserve(assetArray.size());
        for (auto asset : assetArray)
        {
            this->assets.emplace_back(asset.toObject());
        }
    }

    GitHubReleaseAsset assetByName(const QString &assetName) const
    {
        for (const auto &asset : this->assets)
        {
            if (asset.name == assetName)
            {
                return asset;
            }
        }
        return GitHubReleaseAsset();
    }
};

using ReleaseCallback = std::function<void(const GitHubRelease &)>;
using FailureCallback = std::function<void()>;

class GitHubReleases
{
private:
    static const QString RELEASES_API_URL;

    GitHubReleases();

public:
    static void getLatestRelease(ReleaseCallback releaseCallback,
                                 FailureCallback failureCallback);
    static void getReleaseByTag(const QString &tag,
                                ReleaseCallback releaseCallback,
                                FailureCallback failureCallback);

private:
    static NetworkRequest makeRequest(const QString &path);
    static void executeAndHandleCallbacks(const QString &path,
                                          ReleaseCallback releaseCallback,
                                          FailureCallback failureCallback);
};

}  // namespace chatterino
