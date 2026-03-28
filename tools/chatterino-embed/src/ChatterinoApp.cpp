#include "Application.hpp"
#include "ChatterinoAppPrivate.hpp"
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

#include <chatterino-embed/ChatterinoApp.hpp>

namespace chatterino::embed {

namespace {

Args makeArgs(const CreateAppArgs &appArgs)
{
    Args args;
    args.dontSaveSettings = !appArgs.saveSettingsOnExit;
    args.dontLoadMainWindow = true;
    args.isFramelessEmbed = true;
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

class ChatterinoAppPrivate
{
public:
    ChatterinoAppPrivate(const CreateAppArgs &args)
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

    static ChatterinoApp *createPublic(const CreateAppArgs &args)
    {
        return new ChatterinoApp(new ChatterinoAppPrivate(args));
    }

    Paths paths;
    Args args;
    Settings settings;
    Updates updates;
    Application app;
};

ChatterinoApp::ChatterinoApp(ChatterinoAppPrivate *private_, QObject *parent)
    : QObject(parent)
    , private_(private_)
{
}

ChatterinoApp::~ChatterinoApp() = default;

ChatterinoApp *createAppPrivate(const CreateAppArgs &args)
{
    preInitialize();
    return ChatterinoAppPrivate::createPublic(args);
}

}  // namespace chatterino::embed
