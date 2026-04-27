#include "Application.hpp"
#include "AppPrivate.hpp"
#include "common/Args.hpp"
#include "common/Env.hpp"
#include "common/network/NetworkManager.hpp"
#include "providers/IvrApi.hpp"
#include "providers/NetworkConfigurationProvider.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Updates.hpp"

#include <chatterino-embed/App.hpp>

namespace chatterino::embed {

namespace {

Args makeArgs(const CreateAppArgs &appArgs)
{
    Args args;
    args.dontSaveSettings = !appArgs.saveSettingsOnExit;
    args.dontLoadMainWindow = true;
    args.isInjectedEmbed = true;
    return args;
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
        : paths(args.rootDirectory)
        , args(makeArgs(args))
        , settings(this->args, this->paths.settingsDirectory)
        , updates(this->paths, this->settings)
        , app(this->settings, this->paths, this->args, this->updates)
    {
        this->app.initialize(this->settings, this->paths);
        this->app.getTwitch()->connect();

        auto *twitch = dynamic_cast<TwitchIrcServer *>(this->app.getTwitch());
        this->settings.enableBTTVChannelEmotes.connect(
            [twitch] {
                twitch->reloadAllBTTVChannelEmotes();
            },
            false);
        this->settings.enableFFZChannelEmotes.connect(
            [twitch] {
                twitch->reloadAllFFZChannelEmotes();
            },
            false);
        this->settings.enableSevenTVChannelEmotes.connect(
            [twitch] {
                twitch->reloadAllSevenTVChannelEmotes();
            },
            false);
    }

    static App *createPublic(const CreateAppArgs &args)
    {
        return new App(new AppPrivate(args));
    }

    Paths paths;
    Args args;
    Settings settings;
    Updates updates;
    Application app;
};

App::App(AppPrivate *private_, QObject *parent)
    : QObject(parent)
    , private_(private_)
{
}

App::~App() = default;

App *createAppPrivate(const CreateAppArgs &args)
{
    preInitialize();
    return AppPrivate::createPublic(args);
}

}  // namespace chatterino::embed
