#include "GitHubReleases.hpp"

#include "common/Outcome.hpp"
#include "common/QLogging.hpp"

namespace chatterino {

const QString GitHubReleases::RELEASES_API_URL =
    "https://api.github.com/repos/Chatterino/chatterino2/releases";

void GitHubReleases::getLatestRelease(ReleaseCallback releaseCallback,
                                      FailureCallback failureCallback)
{
    GitHubReleases::executeAndHandleCallbacks("/latest", releaseCallback,
                                              failureCallback);
}

void GitHubReleases::getReleaseByTag(const QString &tag,
                                     ReleaseCallback releaseCallback,
                                     FailureCallback failureCallback)
{
    GitHubReleases::executeAndHandleCallbacks("/tags/" + tag, releaseCallback,
                                              failureCallback);
}

NetworkRequest GitHubReleases::makeRequest(const QString &path)
{
    assert(path.startsWith("/"));
    QUrl fullUrl(GitHubReleases::RELEASES_API_URL + path);
    return NetworkRequest(fullUrl).timeout(5 * 1000).header(
        "Accept", "application/vnd.github.v3+json");
}

void GitHubReleases::executeAndHandleCallbacks(const QString &path,
                                               ReleaseCallback releaseCallback,
                                               FailureCallback failureCallback)
{
    GitHubReleases::makeRequest(path)
        .onSuccess([releaseCallback](auto result) -> Outcome {
            QJsonObject root = result.parseJson();
            GitHubRelease release(root);

            releaseCallback(release);

            return Success;
        })
        .onError([failureCallback](auto result) {
            qCWarning(chatterinoUpdate)
                << "Failed to fetch GitHub release:" << result.status()
                << QString(result.getData());

            failureCallback();
        })
        .execute();
}

};  // namespace chatterino
