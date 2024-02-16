#include "common/network/NetworkManager.hpp"

#include <QNetworkAccessManager>

namespace chatterino {

QThread *NetworkManager::workerThread = nullptr;
QNetworkAccessManager *NetworkManager::accessManager = nullptr;

void NetworkManager::init()
{
    assert(!NetworkManager::workerThread);

    NetworkManager::workerThread = new QThread;
    NetworkManager::workerThread->start();

    NetworkManager::accessManager = new QNetworkAccessManager;
    NetworkManager::accessManager->moveToThread(NetworkManager::workerThread);
}

void NetworkManager::deinit()
{
    if (NetworkManager::workerThread)
    {
        NetworkManager::workerThread->quit();
        NetworkManager::workerThread->wait();
    }

    delete NetworkManager::accessManager;
    NetworkManager::accessManager = nullptr;
    delete NetworkManager::workerThread;
    NetworkManager::workerThread = nullptr;
}

}  // namespace chatterino
