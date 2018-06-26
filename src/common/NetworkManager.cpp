#include "common/NetworkManager.hpp"

#include <QNetworkAccessManager>

namespace chatterino {

QThread NetworkManager::workerThread;
QNetworkAccessManager NetworkManager::NaM;

void NetworkManager::init()
{
    NetworkManager::NaM.moveToThread(&NetworkManager::workerThread);
    NetworkManager::workerThread.start();
}

void NetworkManager::deinit()
{
    NetworkManager::workerThread.quit();
    NetworkManager::workerThread.wait();
}

}  // namespace chatterino
