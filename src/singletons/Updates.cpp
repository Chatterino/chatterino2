#include "Updates.hpp"

#include "Settings.hpp"
#include "common/Modes.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/Version.hpp"
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

#ifdef Q_OS_MACOS
    QMessageBox *box = new QMessageBox(
        QMessageBox::Information, "Chatterino Update",
        "A link will open in your browser. Download and install to update.");
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->exec();
    QDesktopServices::openUrl(this->updateExe_);
#elif defined Q_OS_LINUX
    QMessageBox *box =
        new QMessageBox(QMessageBox::Information, "Chatterino Update",
                        "Automatic updates are currently not available on "
                        "linux. Please redownload the app to update.");
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->exec();
    QDesktopServices::openUrl(this->updateGuideLink_);
#elif defined Q_OS_WIN
    if (getPaths()->isPortable())
    {
        QMessageBox *box =
            new QMessageBox(QMessageBox::Information, "Chatterino Update",
                            "Chatterino is downloading the update "
                            "in the background and will run the "
                            "updater once it is finished.");
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();

        NetworkRequest(this->updatePortable_)
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
            .onSuccess([this](auto result) -> Outcome {
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
        QMessageBox *box =
            new QMessageBox(QMessageBox::Information, "Chatterino Update",
                            "Chatterino is downloading the update "
                            "in the background and will run the "
                            "updater once it is finished.");
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->show();

        NetworkRequest(this->updateExe_)
            .timeout(600000)
            .onError([this](NetworkResult) {
                this->setStatus_(DownloadFailed);

                QMessageBox *box = new QMessageBox(
                    QMessageBox::Information, "Chatterino Update",
                    "Failed to download the update. \n\nTry manually "
                    "downloading the update.");
                box->setAttribute(Qt::WA_DeleteOnClose);
                box->exec();
            })
            .onSuccess([this](auto result) -> Outcome {
                QByteArray object = result.getData();
                auto filename =
                    combinePath(getPaths()->miscDirectory, "Update.exe");

                QFile file(filename);
                file.open(QIODevice::Truncate | QIODevice::WriteOnly);

                if (file.write(object) == -1)
                {
                    this->setStatus_(WriteFileFailed);
                    QMessageBox *box = new QMessageBox(
                        QMessageBox::Information, "Chatterino Update",
                        "Failed to save the update file. This could be due to "
                        "window settings or antivirus software.\n\nTry "
                        "manually "
                        "downloading the update.");
                    box->setAttribute(Qt::WA_DeleteOnClose);
                    box->exec();

                    QDesktopServices::openUrl(this->updateExe_);
                    return Failure;
                }
                file.flush();
                file.close();

                if (QProcess::startDetached(filename))
                {
                    QApplication::exit(0);
                }
                else
                {
                    QMessageBox *box = new QMessageBox(
                        QMessageBox::Information, "Chatterino Update",
                        "Failed to execute update binary. This could be due to "
                        "window "
                        "settings or antivirus software.\n\nTry manually "
                        "downloading "
                        "the update.");
                    box->setAttribute(Qt::WA_DeleteOnClose);
                    box->exec();

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
    if (!Version::instance().isSupportedOS())
    {
        qCDebug(chatterinoUpdate)
            << "Update checking disabled because OS doesn't appear to be one "
               "of Windows, GNU/Linux or macOS.";
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
            auto object = result.parseJson();
            /// Version available on every platform
            QJsonValue version_val = object.value("version");

            if (!version_val.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate) << "error updating";
                return Failure;
            }

#if defined Q_OS_WIN || defined Q_OS_MACOS
            /// Downloads an installer for the new version
            QJsonValue updateExe_val = object.value("updateexe");
            if (!updateExe_val.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate) << "error updating";
                return Failure;
            }
            this->updateExe_ = updateExe_val.toString();

#    ifdef Q_OS_WIN
            /// Windows portable
            QJsonValue portable_val = object.value("portable_download");
            if (!portable_val.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate) << "error updating";
                return Failure;
            }
            this->updatePortable_ = portable_val.toString();
#    endif

#elif defined Q_OS_LINUX
            QJsonValue updateGuide_val = object.value("updateguide");
            if (updateGuide_val.isString())
            {
                this->updateGuideLink_ = updateGuide_val.toString();
            }
#else
            return Failure;
#endif

            /// Current version
            this->onlineVersion_ = version_val.toString();

            /// Update available :)
            if (this->currentVersion_ != this->onlineVersion_)
            {
                this->setStatus_(UpdateAvailable);
                this->isDowngrade_ =
                    isDowngradeOf(this->onlineVersion_, this->currentVersion_);
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
