#include "Application.hpp"
#include "AppPrivate.hpp"
#include "common/Args.hpp"
#include "common/Env.hpp"
#include "common/Modes.hpp"
#include "common/network/NetworkManager.hpp"
#include "providers/IvrApi.hpp"
#include "providers/NetworkConfigurationProvider.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Updates.hpp"
#include "util/IpcQueue.hpp"

#include <chatterino-embed/App.hpp>

namespace chatterino::embed {

namespace {

Args makeArgs(const CreateAppArgs &appArgs)
{
    Args args;
    args.dontSaveSettings = !appArgs.saveSettingsOnExit;
    args.dontLoadMainWindow = true;
    args.isInjectedEmbed = true;
    args.portableDirectory = appArgs.rootDirectory;
    args.portableEnable = appArgs.rootDirectory.has_value();
    return args;
}

Modes::Init makeModes(const Args &args)
{
    return {
        .isPortable = args.portableEnable,
        .isExternallyPackaged = true,
    };
}

void preInitialize()
{
    static bool initialized = false;
    if (initialized)
    {
        return;
    }
    initialized = true;

    initResources();
    NetworkConfigurationProvider::applyFromEnv(Env::get());
    IvrApi::initialize();
    Helix::initialize();
    NetworkManager::init();
}

}  // namespace

class AppPrivate
{
public:
    AppPrivate(const CreateAppArgs &args)
        : args(makeArgs(args))
        , modes(makeModes(this->args))
        , paths(this->args, this->modes)
        , settings(this->modes, this->args, this->paths.settingsDirectory)
        , updates(this->modes, this->paths, this->settings)
        , app(this->settings, this->paths, this->args, this->updates)
    {
        ipc::initPaths(&this->paths);
        this->app.initialize(this->settings, this->modes, this->paths);
        this->app.connect();
    }

    static App *createPublic(const CreateAppArgs &args)
    {
        return new App(new AppPrivate(args));
    }

    Args args;
    Modes modes;
    Paths paths;
    Settings settings;
    Updates updates;
    Application app;
};

App::App(AppPrivate *private_, QObject *parent)
    : QObject(parent)
    , private_(private_)
{
}

void App::aboutToQuit()
{
    this->private_->app.aboutToQuit();

    getSettings()->requestSave();
    getSettings()->disableSave();

    this->private_->app.stop();
}

App::~App() = default;

App *createAppPrivate(const CreateAppArgs &args)
{
    preInitialize();
    return AppPrivate::createPublic(args);
}

}  // namespace chatterino::embed
