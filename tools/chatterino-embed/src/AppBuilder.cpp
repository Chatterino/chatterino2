#include "AppPrivate.hpp"

#include <chatterino-embed/AppBuilder.hpp>

namespace chatterino::embed {

class AppBuilderPrivate
{
public:
    QString rootDirectory;
    bool saveSettingsOnExit = false;
};

AppBuilder::AppBuilder(QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<AppBuilderPrivate>())
{
}

AppBuilder::~AppBuilder() = default;

QString AppBuilder::rootDirectory() const
{
    return this->private_->rootDirectory;
}

void AppBuilder::setRootDirectory(const QString &rootDirectory)
{
    this->private_->rootDirectory = rootDirectory;
}

bool AppBuilder::saveSettingsOnExit() const
{
    return this->private_->saveSettingsOnExit;
}

void AppBuilder::setSaveSettingsOnExit(bool value)
{
    this->private_->saveSettingsOnExit = value;
}

App *AppBuilder::createApp()
{
    return createAppPrivate(CreateAppArgs{
        .rootDirectory = this->private_->rootDirectory,
        .saveSettingsOnExit = this->private_->saveSettingsOnExit,
    });
}

}  // namespace chatterino::embed
