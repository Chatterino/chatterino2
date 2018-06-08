#include "updatemanager.hpp"

#include "util/networkrequest.hpp"
#include "version.hpp"

namespace chatterino {
namespace singletons {

UpdateManager::UpdateManager()
    : currentVersion_(CHATTERINO_VERSION)
{
    qDebug() << "init UpdateManager";
}

UpdateManager &UpdateManager::getInstance()
{
    // fourtf: don't add this class to the application class
    static UpdateManager instance;

    return instance;
}

const QString &UpdateManager::getCurrentVersion() const
{
    return currentVersion_;
}

const QString &UpdateManager::getOnlineVersion() const
{
    return onlineVersion_;
}

void UpdateManager::installUpdates()
{
}

void UpdateManager::checkForUpdates()
{
    QString url = "https://notitia.chatterino.com/version/chatterino/" CHATTERINO_OS "/stable";

    util::NetworkRequest req(url);
    req.setTimeout(30000);
    req.getJSON([this](QJsonObject &object) {
        QJsonValue version_val = object.value("version");
        if (!version_val.isString()) {
            this->setStatus_(Error);
            qDebug() << "error updating";
            return;
        }

        this->onlineVersion_ = version_val.toString();

        if (this->currentVersion_ != this->onlineVersion_) {
            this->setStatus_(UpdateAvailable);
        } else {
            this->setStatus_(NoUpdateAvailable);
        }
    });
    this->setStatus_(Searching);
    req.execute();
}

UpdateManager::UpdateStatus UpdateManager::getStatus() const
{
    return this->status_;
}

void UpdateManager::setStatus_(UpdateStatus status)
{
    if (this->status_ != status) {
        this->status_ = status;
        this->statusUpdated.invoke(status);
    }
}

}  // namespace singletons
}  // namespace chatterino
