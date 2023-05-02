#include "Updates.hpp"

#include "common/Modes.hpp"
#include "common/NetworkRequest.hpp"
#include "common/NetworkResult.hpp"
#include "common/Outcome.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "Settings.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/PostToThread.hpp"

#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <semver/semver.hpp>

namespace chatterino {
namespace {
    QString currentBranch()
    {
        return getSettings()->betaUpdates ? "beta" : "stable";
    }

    QMessageBox *createMessageBox(const QString &content)
    {
        auto *box = new QMessageBox(QMessageBox::Information,
                                    "Chatterino Update", content);
        box->setAttribute(Qt::WA_DeleteOnClose);
        return box;
    }

    QMessageBox *showMessageBox(const QString &content)
    {
        auto *box = createMessageBox(content);
        box->show();
        return box;
    }

    void execMessageBox(const QString &content)
    {
        createMessageBox(content)->exec();
    }

}  // namespace

Updates::Updates()
    : currentVersion_(CHATTERINO_VERSION)
    , updateGuideLink_("https://chatterino.com")
{
    qCDebug(chatterinoUpdate) << "init UpdateManager";
}

Updates &Updates::instance()
{
    // fourtf: don't add this class to the application class
    static Updates instance;

    return instance;
}

/// Checks if the online version is newer or older than the current version.
bool Updates::isDowngradeOf(const QString &online, const QString &current)
{
    semver::version onlineVersion;
    if (!onlineVersion.from_string_noexcept(online.toStdString()))
    {
        qCWarning(chatterinoUpdate) << "Unable to parse online version"
                                    << online << "into a proper semver string";
        return false;
    }

    semver::version currentVersion;
    if (!currentVersion.from_string_noexcept(current.toStdString()))
    {
        qCWarning(chatterinoUpdate) << "Unable to parse current version"
                                    << current << "into a proper semver string";
        return false;
    }

    return onlineVersion < currentVersion;
}

const QString &Updates::getCurrentVersion() const
{
    return currentVersion_;
}

const QString &Updates::getOnlineVersion() const
{
    return onlineVersion_;
}

void Updates::installUpdates()
{
    if (this->status_ != UpdateAvailable)
    {
        assert(false);
        return;
    }

#ifdef Q_OS_MACOS
    execMessageBox(
        "A link will open in your browser. Download and install to update.");
    QDesktopServices::openUrl(this->updateExe_);
#elif defined Q_OS_LINUX
    execMessageBox("Automatic updates are currently not available on Linux. "
                   "Please redownload the app to update.");
    QDesktopServices::openUrl(this->updateGuideLink_);
#elif defined Q_OS_WIN
    if (getPaths()->isPortable())
    {
        showMessageBox("Chatterino is downloading the update in the background "
                       "and will run the updater once it is finished.");

        NetworkRequest(this->updatePortable_)
            .timeout(600000)
            .followRedirects()
            .onError([this](auto) {
                this->setStatus_(DownloadFailed);
                showMessageBox("Failed while trying to download the update.")
                    ->raise();
            })
            .onSuccess([this](auto result) -> Outcome {
                if (result.status() != 200)
                {
                    showMessageBox(
                        QStringLiteral("The update couldn't be downloaded "
                                       "(HTTP status %1).")
                            .arg(result.status()));
                    return Failure;
                }

                QByteArray object = result.getData();
                auto filename =
                    combinePath(getPaths()->miscDirectory, "update.zip");

                QFile file(filename);
                file.open(QIODevice::Truncate | QIODevice::WriteOnly);

                if (file.write(object) == -1)
                {
                    this->setStatus_(WriteFileFailed);
                    return Failure;
                }
                file.flush();
                file.close();

                QProcess::startDetached(
                    combinePath(QCoreApplication::applicationDirPath(),
                                "updater.1/ChatterinoUpdater.exe"),
                    {filename, "restart"});

                QApplication::exit(0);
                return Success;
            })
            .execute();
        this->setStatus_(Downloading);
    }
    else
    {
        showMessageBox("Chatterino is downloading the update in the background "
                       "and will run the updater once it is finished.");

        NetworkRequest(this->updateExe_)
            .timeout(600000)
            .followRedirects()
            .onError([this](auto) {
                this->setStatus_(DownloadFailed);

                execMessageBox("Failed to download the update. \n\nTry "
                               "manually downloading the update.");
            })
            .onSuccess([this](auto result) -> Outcome {
                if (result.status() != 200)
                {
                    showMessageBox(
                        QStringLiteral("The update couldn't be downloaded "
                                       "(HTTP status %1).")
                            .arg(result.status()));
                    return Failure;
                }

                QByteArray object = result.getData();
                auto filePath =
                    combinePath(getPaths()->miscDirectory, "Update.exe");

                QFile file(filePath);
                file.open(QIODevice::Truncate | QIODevice::WriteOnly);

                if (file.write(object) == -1)
                {
                    this->setStatus_(WriteFileFailed);
                    execMessageBox(
                        "Failed to save the update file. This could be due to "
                        "Windows settings or antivirus software.\n\nTry "
                        "manually downloading the update.");

                    QDesktopServices::openUrl(this->updateExe_);
                    return Failure;
                }
                file.flush();
                file.close();

                if (QProcess::startDetached(filePath, {}))
                {
                    QApplication::exit(0);
                }
                else
                {
                    execMessageBox(
                        "Failed to execute update binary. This could be due to "
                        "Windows settings or antivirus software.\n\nTry "
                        "manually downloading the update.");

                    QDesktopServices::openUrl(this->updateExe_);
                }

                return Success;
            })
            .execute();
        this->setStatus_(Downloading);
    }
#endif
}

void Updates::checkForUpdates()
{
    auto version = Version::instance();

    if (!version.isSupportedOS())
    {
        qCDebug(chatterinoUpdate)
            << "Update checking disabled because OS doesn't appear to be one "
               "of Windows, GNU/Linux or macOS.";
        return;
    }

    // Disable updates on Flatpak
    if (version.isFlatpak())
    {
        return;
    }

    // Disable updates if on nightly
    if (Modes::instance().isNightly)
    {
        return;
    }

    QString url =
        "https://notitia.chatterino.com/version/chatterino/" CHATTERINO_OS "/" +
        currentBranch();

    NetworkRequest(url)
        .timeout(60000)
        .onSuccess([this](auto result) -> Outcome {
            const auto object = result.parseJson();
            /// Version available on every platform
            auto version = object["version"];

            if (!version.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate)
                    << "error checking version - missing 'version'" << object;
                return Failure;
            }

#if defined Q_OS_WIN || defined Q_OS_MACOS
            /// Downloads an installer for the new version
            auto updateExeUrl = object["updateexe"];
            if (!updateExeUrl.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate)
                    << "error checking version - missing 'updateexe'" << object;
                return Failure;
            }
            this->updateExe_ = updateExeUrl.toString();

#    ifdef Q_OS_WIN
            /// Windows portable
            auto portableUrl = object["portable_download"];
            if (!portableUrl.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate)
                    << "error checking version - missing 'portable_download'"
                    << object;
                return Failure;
            }
            this->updatePortable_ = portableUrl.toString();
#    endif

#elif defined Q_OS_LINUX
            auto updateGuide = object["updateguide"];
            if (updateGuide.isString())
            {
                this->updateGuideLink_ = updateGuide.toString();
            }
#else
            return Failure;
#endif

            /// Current version
            this->onlineVersion_ = version.toString();

            /// Update available :)
            if (this->currentVersion_ != this->onlineVersion_)
            {
                this->setStatus_(UpdateAvailable);
                this->isDowngrade_ = Updates::isDowngradeOf(
                    this->onlineVersion_, this->currentVersion_);
            }
            else
            {
                this->setStatus_(NoUpdateAvailable);
            }
            return Failure;
        })
        .execute();
    this->setStatus_(Searching);
}

Updates::Status Updates::getStatus() const
{
    return this->status_;
}

bool Updates::shouldShowUpdateButton() const
{
    switch (this->getStatus())
    {
        case UpdateAvailable:
        case SearchFailed:
        case Downloading:
        case DownloadFailed:
        case WriteFileFailed:
            return true;

        default:
            return false;
    }
}

bool Updates::isError() const
{
    switch (this->getStatus())
    {
        case SearchFailed:
        case DownloadFailed:
        case WriteFileFailed:
            return true;

        default:
            return false;
    }
}

bool Updates::isDowngrade() const
{
    return this->isDowngrade_;
}

void Updates::setStatus_(Status status)
{
    if (this->status_ != status)
    {
        this->status_ = status;
        postToThread([this, status] {
            this->statusUpdated.invoke(status);
        });
    }
}

}  // namespace chatterino
