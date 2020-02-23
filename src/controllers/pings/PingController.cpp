#include "controllers/pings/PingController.hpp"
#include "controllers/pings/PingModel.hpp"

namespace chatterino {

void PingController::initialize(Settings &settings, Paths &paths)
{
    this->initialized_ = true;
    for (const QString &channelName : this->pingSetting_.getValue())
    {
        this->channelVector.append(channelName);
    }

    this->channelVector.delayedItemsChanged.connect([this] {  //
        this->pingSetting_.setValue(this->channelVector.raw());
    });
}

PingModel *PingController::createModel(QObject *parent)
{
    PingModel *model = new PingModel(parent);
    model->initialize(&this->channelVector);
    return model;
}

bool PingController::isMuted(const QString &channelName)
{
    for (const auto &channel : this->channelVector)
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
    channelVector.append(channelName);
}

void PingController::unmuteChannel(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != channelVector.raw().size(); i++)
    {
        if (channelVector.raw()[i].toLower() == channelName.toLower())
        {
            channelVector.removeAt(i);
            i--;
        }
    }
}

bool PingController::toggleMuteChannel(const QString &channelName)
{
    if (this->isMuted(channelName))
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
