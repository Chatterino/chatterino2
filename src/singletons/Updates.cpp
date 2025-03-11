#include "singletons/Updates.hpp"

#include "common/Literals.hpp"
#include "common/Modes.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/PostToThread.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QMessageBox>
#include <QProcess>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QtConcurrent>
#include <semver/semver.hpp>

namespace {

using namespace chatterino;
using namespace literals;

QString currentBranch()
{
    return getSettings()->betaUpdates ? "beta" : "stable";
}

#if defined(Q_OS_WIN)
const QString CHATTERINO_OS = u"win"_s;
#elif defined(Q_OS_MACOS)
const QString CHATTERINO_OS = u"macos"_s;
#elif defined(Q_OS_LINUX)
const QString CHATTERINO_OS = u"linux"_s;
#elif defined(Q_OS_FREEBSD)
const QString CHATTERINO_OS = u"freebsd"_s;
#else
const QString CHATTERINO_OS = u"unknown"_s;
#endif

}  // namespace

namespace chatterino {

Updates::Updates(const Paths &paths_, Settings &settings)
    : paths(paths_)
    , currentVersion_(CHATTERINO_VERSION)
    , updateGuideLink_("https://chatterino.com")
{
    qCDebug(chatterinoUpdate) << "init UpdateManager";

    settings.betaUpdates.connect(
        [this] {
            this->checkForUpdates();
        },
        this->managedConnections, false);
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

void Updates::deleteOldFiles()
{
    std::ignore = QtConcurrent::run([dir{this->paths.miscDirectory}] {
        {
            auto path = combinePath(dir, "Update.exe");
            if (QFile::exists(path))
            {
                QFile::remove(path);
            }
        }
        {
            auto path = combinePath(dir, "update.zip");
            if (QFile::exists(path))
            {
                QFile::remove(path);
            }
        }
    });
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
    box->open();
    QDesktopServices::openUrl(this->updateExe_);
#elif defined Q_OS_LINUX
    QMessageBox *box =
        new QMessageBox(QMessageBox::Information, "Chatterino Update",
                        "Automatic updates are currently not available on "
                        "Linux. Please redownload the app to update.");
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->open();
    QDesktopServices::openUrl(this->updateGuideLink_);
#elif defined Q_OS_WIN
    if (Modes::instance().isPortable)
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
            .onSuccess([this](auto result) {
                if (result.status() != 200)
                {
                    auto *box = new QMessageBox(
                        QMessageBox::Information, "Chatterino Update",
                        QStringLiteral("The update couldn't be downloaded "
                                       "(Error: %1).")
                            .arg(result.formatError()));
                    box->setAttribute(Qt::WA_DeleteOnClose);
                    box->exec();
                    return;
                }

                QByteArray object = result.getData();
                auto filename =
                    combinePath(this->paths.miscDirectory, "update.zip");

                QFile file(filename);
                file.open(QIODevice::Truncate | QIODevice::WriteOnly);

                if (file.write(object) == -1)
                {
                    this->setStatus_(WriteFileFailed);
                    return;
                }
                file.flush();
                file.close();

                QProcess::startDetached(
                    combinePath(QCoreApplication::applicationDirPath(),
                                "updater.1/ChatterinoUpdater.exe"),
                    {filename, "restart"});

                QApplication::exit(0);
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
            .onSuccess([this](auto result) {
                if (result.status() != 200)
                {
                    auto *box = new QMessageBox(
                        QMessageBox::Information, "Chatterino Update",
                        QStringLiteral("The update couldn't be downloaded "
                                       "(Error: %1).")
                            .arg(result.formatError()));
                    box->setAttribute(Qt::WA_DeleteOnClose);
                    box->exec();
                    return;
                }

                QByteArray object = result.getData();
                auto filePath =
                    combinePath(this->paths.miscDirectory, "Update.exe");

                QFile file(filePath);
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
                    return;
                }
                file.flush();
                file.close();

                if (QProcess::startDetached(filePath, {}))
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
            })
            .execute();
        this->setStatus_(Downloading);
    }
#endif
}

void Updates::checkForUpdates()
{
#ifndef CHATTERINO_DISABLE_UPDATER
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

    QString url = "https://notitia.chatterino.com/version/chatterino/" %
                  CHATTERINO_OS % "/" % currentBranch();

    NetworkRequest(url)
        .timeout(60000)
        .onSuccess([this](auto result) {
            const auto object = result.parseJson();
            /// Version available on every platform
            auto version = object["version"];

            if (!version.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate)
                    << "error checking version - missing 'version'" << object;
                return;
            }

#    if defined Q_OS_WIN || defined Q_OS_MACOS
            /// Downloads an installer for the new version
            auto updateExeUrl = object["updateexe"];
            if (!updateExeUrl.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate)
                    << "error checking version - missing 'updateexe'" << object;
                return;
            }
            this->updateExe_ = updateExeUrl.toString();

#        ifdef Q_OS_WIN
            /// Windows portable
            auto portableUrl = object["portable_download"];
            if (!portableUrl.isString())
            {
                this->setStatus_(SearchFailed);
                qCDebug(chatterinoUpdate)
                    << "error checking version - missing 'portable_download'"
                    << object;
                return;
            }
            this->updatePortable_ = portableUrl.toString();
#        endif

#    elif defined Q_OS_LINUX
            QJsonValue updateGuide = object.value("updateguide");
            if (updateGuide.isString())
            {
                this->updateGuideLink_ = updateGuide.toString();
            }
#    else
            return;
#    endif

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
        })
        .execute();
    this->setStatus_(Searching);
#endif
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
