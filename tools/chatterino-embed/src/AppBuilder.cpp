#include "AppPrivate.hpp"

#include <chatterino-embed/AppBuilder.hpp>

namespace chatterino::embed {

class AppBuilderPrivate
{
public:
    CreateAppArgs args;
};

AppBuilder::AppBuilder(QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<AppBuilderPrivate>())
{
}

AppBuilder::~AppBuilder() = default;

std::optional<QString> AppBuilder::rootDirectory() const
{
    return this->private_->args.rootDirectory;
}

void AppBuilder::setRootDirectory(std::optional<QString> rootDirectory)
{
    this->private_->args.rootDirectory = std::move(rootDirectory);
}

bool AppBuilder::saveSettingsOnExit() const
{
    return this->private_->args.saveSettingsOnExit;
}

void AppBuilder::setSaveSettingsOnExit(bool value)
{
    this->private_->args.saveSettingsOnExit = value;
}

App *AppBuilder::createApp()
{
    return createAppPrivate(this->private_->args);
}

}  // namespace chatterino::embed
