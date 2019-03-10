#pragma once

#include <QString>
#include <pajlada/signals/signal.hpp>

namespace chatterino
{
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
        static Updates& getInstance();

        void checkForUpdates();
        const QString& getCurrentVersion() const;
        const QString& getOnlineVersion() const;
        void installUpdates();
        Status getStatus() const;

        bool shouldShowUpdateButton() const;
        bool isError() const;

        pajlada::Signals::Signal<Status> statusUpdated;

    private:
        QString currentVersion_;
        QString onlineVersion_;
        Status status_ = None;

        QString updateUrl_;

        void setStatus_(Status status);
    };

}  // namespace chatterino
