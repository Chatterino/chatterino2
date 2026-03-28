#include "ChatterinoAppPrivate.hpp"

#include <chatterino-embed/ChatterinoAppBuilder.hpp>

namespace chatterino::embed {

class ChatterinoAppBuilderPrivate
{
public:
    QString rootDirectory;
    bool saveSettingsOnExit = false;
};

ChatterinoAppBuilder::ChatterinoAppBuilder(QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<ChatterinoAppBuilderPrivate>())
{
}

ChatterinoAppBuilder::~ChatterinoAppBuilder() = default;

QString ChatterinoAppBuilder::rootDirectory() const
{
    return this->private_->rootDirectory;
}

void ChatterinoAppBuilder::setRootDirectory(const QString &rootDirectory)
{
    this->private_->rootDirectory = rootDirectory;
}

bool ChatterinoAppBuilder::saveSettingsOnExit() const
{
    return this->private_->saveSettingsOnExit;
}

void ChatterinoAppBuilder::setSaveSettingsOnExit(bool value)
{
    this->private_->saveSettingsOnExit = value;
}

ChatterinoApp *ChatterinoAppBuilder::createApp()
{
    return createAppPrivate(CreateAppArgs{
        .rootDirectory = this->private_->rootDirectory,
        .saveSettingsOnExit = this->private_->saveSettingsOnExit,
    });
}

}  // namespace chatterino::embed
