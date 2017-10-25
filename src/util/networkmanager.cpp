#include "util/networkmanager.hpp"
#include "emotemanager.hpp"
#include "messages/lazyloadedimage.hpp"
#include "windowmanager.hpp"

#include <QApplication>
#include <QBuffer>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace chatterino {
namespace util {

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

}  // namespace util
}  // namespace chatterino
