#pragma once

#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signal.hpp>
#include <QString>

#include <memory>
#include <vector>

namespace chatterino {

class Paths;
class Settings;

/**
 * To check for updates, use the `checkForUpdates` method.
 * The class by itself does not start any automatic updates.
 */
class Updates
{
    const Paths &paths;

public:
    Updates(const Paths &paths_, Settings &settings);

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

    static bool isDowngradeOf(const QString &online, const QString &current);

    /**
     * @brief Delete old files that belong to the update process
     */
    void deleteOldFiles();

    void checkForUpdates();
    const QString &getCurrentVersion() const;
    const QString &getOnlineVersion() const;
    void installUpdates();
    Status getStatus() const;

    bool shouldShowUpdateButton() const;
    bool isError() const;
    bool isDowngrade() const;

    pajlada::Signals::Signal<Status> statusUpdated;

private:
    QString currentVersion_;
    QString onlineVersion_;
    Status status_ = None;
    bool isDowngrade_{};

    QString updateExe_;
    QString updatePortable_;
    QString updateGuideLink_;

    void setStatus_(Status status);

    std::vector<std::unique_ptr<pajlada::Signals::ScopedConnection>>
        managedConnections;
};

}  // namespace chatterino
