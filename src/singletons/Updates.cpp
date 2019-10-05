#include "Updates.hpp"

#include "Settings.hpp"
#include "common/Modes.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/Version.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/PostToThread.hpp"

#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>

namespace chatterino {
namespace {
    QString currentBranch()
    {
        return getSettings()->betaUpdates ? "beta" : "stable";
    }
}  // namespace

Updates::Updates()
    : currentVersion_(CHATTERINO_VERSION)
    , updateGuideLink_("https://chatterino.com")
{
    qDebug() << "init UpdateManager";
}

Updates &Updates::getInstance()
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
    // Disable updates if on nightly and windows.
#ifdef Q_OS_WIN
    if (Modes::getInstance().isNightly)
    {
        return;
    }
#endif

    if (Modes::getInstance().test == "x")
    {
        return;
    }

    QString url =
        "https://notitia.chatterino.com/version/chatterino/" CHATTERINO_OS "/" +
        currentBranch();

    NetworkRequest(url)
        .timeout(60000)
        .onSuccess([this](auto result) -> Outcome {
            if (Modes::getInstance().test == "0")
            {
                return Success;
            }
            if (Modes::getInstance().test == "1")
            {
                return Failure;
            }

            auto object = result.parseJson();
            /// Version available on every platform
            QJsonValue version_val = object.value("version");

            if (!version_val.isString())
            {
                this->setStatus_(SearchFailed);
                qDebug() << "error updating";
                return Failure;
            }
            if (Modes::getInstance().test == "2")
            {
                return Failure;
            }

#if defined Q_OS_WIN || defined Q_OS_MACOS
            /// Windows downloads an installer for the new version
            QJsonValue updateExe_val = object.value("updateexe");
            if (!updateExe_val.isString())
            {
                this->setStatus_(SearchFailed);
                qDebug() << "error updating";
                return Failure;
            }
            this->updateExe_ = updateExe_val.toString();
            if (Modes::getInstance().test == "3")
            {
                return Failure;
            }

            /// Windows portable
            QJsonValue portable_val = object.value("portable_download");
            if (!portable_val.isString())
            {
                this->setStatus_(SearchFailed);
                qDebug() << "error updating";
                return Failure;
            }
            this->updatePortable_ = portable_val.toString();

#elif defined Q_OS_LINUX
            QJsonValue updateGuide_val = object.value("updateguide");
            if (updateGuide_val.isString())
            {
                this->updateGuideLink_ = updateGuide_val.toString();
            }
#else
            return Failure;
#endif

            if (Modes::getInstance().test == "4")
            {
                return Failure;
            }

            /// Current version
            this->onlineVersion_ = version_val.toString();

            if (Modes::getInstance().test == "5")
            {
                return Failure;
            }

            /// Update available :)
            if (this->currentVersion_ != this->onlineVersion_)
            {
                this->setStatus_(UpdateAvailable);
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
    if (Modes::getInstance().test == "8")
    {
        return false;
    }

    if (Modes::getInstance().test == "9")
    {
        return true;
    }

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

void Updates::setStatus_(Status status)
{
    if (this->status_ != status)
    {
        this->status_ = status;

        if (Modes::getInstance().test == "6")
        {
            return;
        }

        postToThread([this, status] { this->statusUpdated.invoke(status); });
    }
}

}  // namespace chatterino
