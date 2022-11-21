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

class GitHubReleases
{
    using ReleasePredicate = std::function<bool(const GitHubRelease &)>;
    using ReleaseCallback = std::function<void(const GitHubRelease &)>;
    using FailureCallback = std::function<void()>;

private:
    static const QString RELEASES_API_URL;

    GitHubReleases();

public:
    /**
     * @brief Get the most recent release that was not a prerelease
     *
     * @param releaseCallback Callback on got matching release
     * @param failureCallback Callback on failure
     */
    static void getLatestNotPrerelease(ReleaseCallback releaseCallback,
                                       FailureCallback failureCallback);
    /**
     * @brief Get the most recent release that was a prerelease
     *
     * @param releaseCallback Callback on got matching prerelease
     * @param failureCallback Callback on failure
     */
    static void getLatestPrerelease(ReleaseCallback releaseCallback,
                                    FailureCallback failureCallback);
    /**
     * @brief Get the latest release
     *
     * @param releaseCallback Callback on got release
     * @param failureCallback Callback on failure
     */
    static void getLatestRelease(ReleaseCallback releaseCallback,
                                 FailureCallback failureCallback);
    /**
     * @brief Get a release by its Git tag
     *
     * @param tag Git tag to query
     * @param releaseCallback Callback on got release
     * @param failureCallback Callback on failure
     */
    static void getReleaseByTag(const QString &tag,
                                ReleaseCallback releaseCallback,
                                FailureCallback failureCallback);

private:
    static NetworkRequest makeRequest(const QString &path);
    static void executeAndHandleCallbacks(const QString &path,
                                          ReleaseCallback releaseCallback,
                                          FailureCallback failureCallback);
    static void executeAndHandleFirstMatching(ReleasePredicate predicate,
                                              ReleaseCallback releaseCallback,
                                              FailureCallback failureCallback);
};

}  // namespace chatterino
