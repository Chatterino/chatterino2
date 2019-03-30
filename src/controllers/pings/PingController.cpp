#include "controllers/pings/PingController.hpp"
#include "controllers/pings/PingModel.hpp"

namespace chatterino {

void PingController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->pingSetting_.getValue())
    {
        this->channelVector.appendItem(channelName);
    }

    this->channelVector.delayedItemsChanged.connect([this] {  //
        this->pingSetting_.setValue(this->channelVector.getVector());
    });
}

PingModel *PingController::createModel(QObject *parent)
{
    PingModel *model = new PingModel(parent);
    model->init(&this->channelVector);
    return model;
}

bool PingController::isMuted(const QString &channelName)
{
    for (const auto &channel : this->channelVector.getVector())
    {
        if (channelName.toLower() == channel.toLower())
        {
            return true;
        }
    }
    return false;
}

void PingController::muteChannel(const QString &channelName)
{
    channelVector.appendItem(channelName);
}

void PingController::unmuteChannel(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != channelVector.getVector().size(); i++)
    {
        if (channelVector.getVector()[i].toLower() == channelName.toLower())
        {
            channelVector.removeItem(i);
            i--;
        }
    }
}

bool PingController::toggleMuteChannel(const QString &channelName)
{
    if (isMuted(channelName))
    {
        unmuteChannel(channelName);
        return false;
    }
    else
    {
        muteChannel(channelName);
        return true;
    }
}

}  // namespace chatterino
