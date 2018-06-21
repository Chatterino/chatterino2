#pragma once

#include <QString>
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace singletons {

class UpdateManager
{
    UpdateManager();

public:
    enum UpdateStatus {
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
    static UpdateManager &getInstance();

    void checkForUpdates();
    const QString &getCurrentVersion() const;
    const QString &getOnlineVersion() const;
    void installUpdates();
    UpdateStatus getStatus() const;

    pajlada::Signals::Signal<UpdateStatus> statusUpdated;

private:
    QString currentVersion_;
    QString onlineVersion_;
    UpdateStatus status_ = None;

    QString updateUrl_;

    void setStatus_(UpdateStatus status);
};

}  // namespace singletons
}  // namespace chatterino
