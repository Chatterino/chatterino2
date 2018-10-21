#include "Updates.hpp"

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "common/Version.hpp"
#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/PostToThread.hpp"

#include <QDebug>
#include <QMessageBox>
#include <QProcess>

namespace chatterino {

Updates::Updates()
    : currentVersion_(CHATTERINO_VERSION)
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

#ifdef Q_OS_WIN
    QMessageBox *box =
        new QMessageBox(QMessageBox::Information, "Chatterino Update",
                        "Chatterino is downloading the update "
                        "in the background and will run the "
                        "updater once it is finished.");
    box->setAttribute(Qt::WA_DeleteOnClose);
    box->show();

    NetworkRequest req(this->updateUrl_);
    req.setTimeout(600000);
    req.onError([this](int) -> bool {
        this->setStatus_(DownloadFailed);

        postToThread([] {
            QMessageBox *box =
                new QMessageBox(QMessageBox::Information, "Chatterino Update",
                                "Failed while trying to download the update.");
            box->setAttribute(Qt::WA_DeleteOnClose);
            box->show();
            box->raise();
        });

        return true;
    });

    req.onSuccess([this](auto result) -> Outcome {
        QByteArray object = result.getData();
        auto filename = combinePath(getPaths()->miscDirectory, "update.zip");

        QFile file(filename);
        file.open(QIODevice::Truncate | QIODevice::WriteOnly);

        if (file.write(object) == -1)
        {
            this->setStatus_(WriteFileFailed);
            return Failure;
        }

        QProcess::startDetached(
            combinePath(QCoreApplication::applicationDirPath(),
                        "updater.1/ChatterinoUpdater.exe"),
            {filename, "restart"});

        QApplication::exit(0);
        return Success;
    });
    this->setStatus_(Downloading);
    req.execute();
#endif
}

void Updates::checkForUpdates()
{
#ifdef Q_OS_WIN
    QString url =
        "https://notitia.chatterino.com/version/chatterino/" CHATTERINO_OS
        "/stable";

    NetworkRequest req(url);
    req.setTimeout(30000);
    req.onSuccess([this](auto result) -> Outcome {
        auto object = result.parseJson();
        QJsonValue version_val = object.value("version");
        QJsonValue update_val = object.value("update");

        if (!version_val.isString() || !update_val.isString())
        {
            this->setStatus_(SearchFailed);
            qDebug() << "error updating";

            postToThread([] {
                QMessageBox *box = new QMessageBox(
                    QMessageBox::Information, "Chatterino Update",
                    "Error while searching for updates.\n\nEither the service "
                    "is down "
                    "temporarily or everything is broken.");
                box->setAttribute(Qt::WA_DeleteOnClose);
                box->show();
                box->raise();
            });
            return Failure;
        }

        this->onlineVersion_ = version_val.toString();
        this->updateUrl_ = update_val.toString();

        if (this->currentVersion_ != this->onlineVersion_)
        {
            this->setStatus_(UpdateAvailable);
            postToThread([this] {
                QMessageBox *box = new QMessageBox(
                    QMessageBox::Information, "Chatterino Update",
                    "An update for chatterino is available.\n\nDo you "
                    "want to download and install it?",
                    QMessageBox::Yes | QMessageBox::No);
                box->setAttribute(Qt::WA_DeleteOnClose);
                box->show();
                box->raise();
                if (box->exec() == QMessageBox::Yes)
                {
                    this->installUpdates();
                }
            });
        }
        else
        {
            this->setStatus_(NoUpdateAvailable);
        }
        return Failure;
    });
    this->setStatus_(Searching);
    req.execute();
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

void Updates::setStatus_(Status status)
{
    if (this->status_ != status)
    {
        this->status_ = status;
        postToThread([this, status] { this->statusUpdated.invoke(status); });
    }
}

}  // namespace chatterino
