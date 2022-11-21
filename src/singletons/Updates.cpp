#include "Updates.hpp"

#include "Settings.hpp"
#include "common/Env.hpp"
#include "common/Modes.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/Version.hpp"
#include "providers/GitHubReleases.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/PostToThread.hpp"

#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include "common/QLogging.hpp"

namespace chatterino {
namespace {
    QString currentBranch()
    {
        return getSettings()->betaUpdates ? "beta" : "stable";
    }

    QString cleanVersion(QString version)
    {
        if (version.startsWith('v'))
        {
            version.remove(0, 1);
        }
        return version;
    }

    bool sameVersion(QString a, QString b)
    {
        if (a.startsWith('v'))
        {
            a.remove(0, 1);
        }
        if (b.startsWith('v'))
        {
            b.remove(0, 1);
        }
        return a == b;
    }

    /// Checks if the online version is newer or older than the current version.
    bool isDowngradeOf(const QString &online, const QString &current)
    {
        static auto matchVersion =
            QRegularExpression(R"((\d+)(?:\.(\d+))?(?:\.(\d+))?(?:\.(\d+))?)");

        // Versions are just strings, they don't need to follow a specific
        // format so we can only assume if one version is newer than another
        // one.

        // We match x.x.x.x with each version level being optional.

        auto onlineMatch = matchVersion.match(online);
        auto currentMatch = matchVersion.match(current);

        for (int i = 1; i <= 4; i++)
        {
            if (onlineMatch.captured(i).toInt() <
                currentMatch.captured(i).toInt())
            {
                return true;
            }
            if (onlineMatch.captured(i).toInt() >
                currentMatch.captured(i).toInt())
            {
                break;
            }
        }

        return false;
    }

    void showPopupMessage(QMessageBox::Icon icon, const QString &title,
                          const QString &body, bool block)
    {
        QMessageBox *box = new QMessageBox(icon, title, body);
        box->setAttribute(Qt::WA_DeleteOnClose);
        if (block)
        {
            box->exec();
        }
        else
        {
            box->show();
        }
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

    if (this->releaseAsset_.downloadURL.isEmpty())
    {
        // We couldn't select the asset download URL, or the platform doesn't support
        // downloading automatically (linux)

#ifdef Q_OS_LINUX
        QMessageBox *box =
            new QMessageBox(QMessageBox::Information, "Chatterino Update",
                            "Automatic updates are currently not available on "
                            "Linux. Please re-download the app to update.");
#else
        QMessageBox *box = new QMessageBox(
            QMessageBox::Information, "Chatterino Update",
            "Chatterino couldn't determine which release to download. Please "
            "manually download the app to update.");

#endif
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->exec();

        QDesktopServices::openUrl(this->updateGuideLink_);
        return;
    }

    showPopupMessage(QMessageBox::Information, "Chatterino Update",
                     "Chatterino is downloading the update "
                     "in the background and will run the "
                     "updater once it is finished.",
                     false);

    qCDebug(chatterinoUpdate) << "Downloading update from asset url:"
                              << this->releaseAsset_.downloadURL;

    bool portable = getPaths()->isPortable();
    this->setStatus_(Downloading);
    NetworkRequest(this->releaseAsset_.downloadURL)
        .followRedirects(true)
        .timeout(600000)
        .onError([this](NetworkResult) {
            this->setStatus_(DownloadFailed);

            postToThread([] {
                QMessageBox *box = new QMessageBox(
                    QMessageBox::Information, "Chatterino Update",
                    "Failed while trying to download the update.");
                box->setAttribute(Qt::WA_DeleteOnClose);
                box->show();
                box->raise();
            });
        })
        .onSuccess([this, portable](auto result) -> Outcome {
            QByteArray object = result.getData();
            QString filePath;
#ifdef Q_OS_WIN
            if (portable)
            {
                filePath = combinePath(getPaths()->miscDirectory, "update.zip");
            }
            else
            {
                filePath = combinePath(getPaths()->miscDirectory, "Update.exe");
            }
#elif defined Q_OS_MAC
            filePath = combinePath(getPaths()->miscDirectory, "Chatterino.dmg");
#else
            return Failure;  // shouldn't happen, but guard against it anyways
#endif

            QFile file(filePath);
            file.open(QIODevice::Truncate | QIODevice::WriteOnly);

            auto onInstallError = [this]() {
                showPopupMessage(
                    QMessageBox::Critical, "Chatterino Update",
                    "Failed to save the update file. This could be due to "
                    "window settings or antivirus software.\n\nA link will "
                    "open allowing you to manually download the update.",
                    true);

                QDesktopServices::openUrl(this->releaseURL_);
            };

            // Write data to disk
            qCDebug(chatterinoUpdate) << "Writing update file to" << filePath;
            if (file.write(object) == -1)
            {
                qCWarning(chatterinoUpdate)
                    << "Failed to write update artifact to disk";

                onInstallError();
                this->setStatus_(WriteFileFailed);
                return Failure;
            }
            file.flush();
            file.close();

#ifdef Q_OS_WIN
            if (portable)
            {
                qCDebug(chatterinoUpdate)
                    << "Starting portable updater" << filePath;
                QProcess::startDetached(
                    combinePath(QCoreApplication::applicationDirPath(),
                                "updater.1/ChatterinoUpdater.exe"),
                    {filePath, "restart"});

                QApplication::exit(0);
                return Success;
            }
            else
            {
                // Attempt to launch installer
                qCDebug(chatterinoUpdate)
                    << "Starting updater executable" << filePath;
                if (QProcess::startDetached(filePath, {}))
                {
                    QApplication::exit(0);
                    return Success;
                }
                else
                {
                    this->setStatus_(Status::None);
                    showPopupMessage(
                        QMessageBox::Warning, "Chatterino Update",
                        "The updater was downloaded successfully but "
                        "Chatterino wasn't able to start it automatically.\n\n"
                        "An explorer window will open with the Updater "
                        "executable. Run this executable to finish the update.",
                        true);

                    // Open explorer with the executable highlighted
                    QString showExecutableCommand =
                        "C:\\Windows\\explorer.exe /select," +
                        QDir::toNativeSeparators(filePath);
                    bool openedExplorer =
                        QProcess::startDetached(showExecutableCommand);

                    if (!openedExplorer)
                    {
                        showPopupMessage(
                            QMessageBox::Warning, "Chatterino Update",
                            "Well, this is embarrassing. Chatterino was also "
                            "unable to open the explorer window. A link will "
                            "open allowing you to manually download the "
                            "update.",
                            true);
                        QDesktopServices::openUrl(this->releaseURL_);
                        return Failure;
                    }
                    return Success;
                }
            }
#elif defined Q_OS_MACOS
            this->setStatus_(Status::None);
            showPopupMessage(
                QMessageBox::Information, "Chatterino Update",
                "Finished downloading the Chatterino update. You may need to "
                "close Chatterino before copying the updated version into your "
                "Applications folder.",
                true);

            QUrl fileUrl = QUrl("file://" + filePath, QUrl::TolerantMode);
            QDesktopServices::openUrl(fileUrl);
            return Success;
#else
            return Success;
#endif
        })
        .execute();
}

// Selects the appropriate asset for the current platform
void Updates::selectReleaseAsset(const GitHubRelease &release)
{
    QString assetName;

#ifdef Q_OS_LINUX
    return;  // Linux automatic updates are not supported
#elif defined Q_OS_MACOS
    assetName = "Chatterino.dmg";
#elif defined Q_OS_WIN
    if (getPaths()->isPortable())
    {
        assetName = "Chatterino.Portable.zip";
    }
    else
    {
        assetName = "Chatterino.Installer.exe";
    }
#endif

    this->releaseAsset_ = release.assetByName(assetName);
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
        // TODO: Download nightly-build tag?
        return;
    }

    GitHubReleases::getLatestRelease(
        [this](const GitHubRelease &release) {
            this->onlineVersion_ = cleanVersion(release.tagName);
            this->releaseURL_ = release.htmlURL;

            if (!sameVersion(this->currentVersion_, this->onlineVersion_) ||
                Env::get().forceUpdate)
            {
                this->selectReleaseAsset(release);
                this->isDowngrade_ =
                    isDowngradeOf(this->onlineVersion_, this->currentVersion_);

                this->setStatus_(UpdateAvailable);
            }
            else
            {
                this->setStatus_(NoUpdateAvailable);
            }
        },
        [this]() {
            this->setStatus_(SearchFailed);
            qCWarning(chatterinoUpdate)
                << "error getting latest release from GitHub";
        });

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
