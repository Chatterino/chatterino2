#include "common/network/NetworkManager.hpp"

#include <QNetworkAccessManager>

namespace chatterino {

QThread *NetworkManager::workerThread = nullptr;
QNetworkAccessManager *NetworkManager::accessManager = nullptr;

void NetworkManager::init()
{
    assert(!NetworkManager::workerThread);
    assert(!NetworkManager::accessManager);

    NetworkManager::workerThread = new QThread;
    NetworkManager::workerThread->setObjectName("NetworkWorker");
    NetworkManager::workerThread->start();

    NetworkManager::accessManager = new QNetworkAccessManager;
    NetworkManager::accessManager->moveToThread(NetworkManager::workerThread);
}

void NetworkManager::deinit()
{
    assert(NetworkManager::workerThread);
    assert(NetworkManager::accessManager);

    // delete the access manager first:
    // - put the event on the worker thread
    // - wait for it to process
    NetworkManager::accessManager->deleteLater();
    NetworkManager::accessManager = nullptr;

    if (NetworkManager::workerThread)
    {
        NetworkManager::workerThread->quit();
        NetworkManager::workerThread->wait();
    }

    NetworkManager::workerThread->deleteLater();
    NetworkManager::workerThread = nullptr;
}

}  // namespace chatterino
