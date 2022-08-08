#pragma once

#include "providers/GitHubReleases.hpp"

#include <QString>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class Updates
{
    Updates();

public:
    enum Status {
        None,
        Searching,
        UpdateAvailable,
        NoUpdateAvailable,
        SearchFailed,
        Downloading,
        DownloadFailed,
        WriteFileFailed,
    };

    // fourtf: don't add this class to the application class
    static Updates &instance();

    void checkForUpdates();
    const QString &getCurrentVersion() const;
    const QString &getOnlineVersion() const;
    void installUpdates();
    Status getStatus() const;

    bool shouldShowUpdateButton() const;
    bool isDowngrade() const;

    pajlada::Signals::Signal<Status> statusUpdated;

private:
    QString currentVersion_;
    QString onlineVersion_;
    Status status_ = None;
    bool isDowngrade_{};

    QString updateGuideLink_;
    QString releaseURL_;

    GitHubReleaseAsset releaseAsset_;
    void selectReleaseAsset(const GitHubRelease &release);

    void setStatus_(Status status);
};

}  // namespace chatterino
