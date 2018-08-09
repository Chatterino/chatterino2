#include "controllers/notifications/NotificationController.hpp"

#include "Application.hpp"
#include "controllers/notifications/NotificationModel.hpp"

namespace chatterino {

NotificationController::NotificationController()
{
}
void NotificationController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->notificationSetting_.getValue()) {
        this->notificationVector.appendItem(channelName);
    }

    this->notificationVector.delayedItemsChanged.connect([this] {  //
        this->notificationSetting_.setValue(
            this->notificationVector.getVector());
    });
}

void NotificationController::updateChannelNotification(
    const QString &channelName)
{
    if (isChannelNotified(channelName)) {
        removeChannelNotification(channelName);
    } else {
        addChannelNotification(channelName);
    }
}

bool NotificationController::isChannelNotified(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != notificationVector.getVector().size(); i++) {
        if (notificationVector.getVector()[i] == channelName) {
            return true;
        }
    }
    return false;
    /*
    const auto &vector = notificationSetting_.getValue();
    return std::find(vector.begin(), vector.end(), channelName) != vector.end();
    */
}

void NotificationController::addChannelNotification(const QString &channelName)
{
    notificationVector.appendItem(channelName);
    /*
    auto vector = notificationSetting_.getValue();
    vector.push_back(channelName);
    notificationSetting_.setValue(vector);
    */
}

void NotificationController::removeChannelNotification(
    const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != notificationVector.getVector().size(); i++) {
        if (notificationVector.getVector()[i] == channelName) {
            notificationVector.removeItem(i);
            i--;
        }
    }

    // notificationVector.removeItem(std::find(
    //    notificationVector.begin(), notificationVector.end(), channelName));
    /*
    auto vector = notificationSetting_.getValue();
    vector.erase(std::find(vector.begin(), vector.end(), channelName));
    notificationSetting_.setValue(vector);
    */
}

NotificationModel *NotificationController::createModel(QObject *parent)
{
    NotificationModel *model = new NotificationModel(parent);
    model->init(&this->notificationVector);
    return model;
}

}  // namespace chatterino
