#include "updatemanager.hpp"

#include "util/networkrequest.hpp"
#include "version.hpp"

namespace chatterino {
namespace singletons {

UpdateManager::UpdateManager()
    : currentVersion(CHATTERINO_VERSION)
{
}

UpdateManager &UpdateManager::getInstance()
{
    static UpdateManager instance;
    return instance;
}

const QString &UpdateManager::getCurrentVersion() const
{
    return this->getCurrentVersion();
}

const QString &UpdateManager::getOnlineVersion() const
{
    return this->getOnlineVersion();
}

void UpdateManager::checkForUpdates()
{
    QString url = "https://notitia.chatterino.com/version/chatterino/" CHATTERINO_OS "/stable";

    util::NetworkRequest req(url);
    req.setTimeout(20000);
    req.getJSON([this](QJsonDocument &document) {
        if (document.isEmpty()) {
            qDebug() << "error updating";
            return;
        }

        QJsonValue version_val = document.object().value("version");
        if (!version_val.isString()) {
            qDebug() << "error updating";
            return;
        }

        this->onlineVersion = version_val.toString();
        this->onlineVersionUpdated.invoke();
    });
}

}  // namespace singletons
}  // namespace chatterino
