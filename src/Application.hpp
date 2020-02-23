#pragma once

#include <QApplication>
#include <memory>

#include "common/SignalVector.hpp"
#include "common/Singleton.hpp"
#include "singletons/NativeMessaging.hpp"

namespace chatterino {

class TwitchIrcServer;
class PubSub;

class CommandController;
class HighlightController;
class IgnoreController;
class TaggedUsersController;
class AccountController;
class ModerationActions;
class NotificationController;
class MutedChannelController;

class Theme;
class WindowManager;
class Logging;
class Paths;
class AccountManager;
class Emotes;
class Settings;
class Fonts;
class Toasts;
class ChatterinoBadges;

class HighlightPhrase;
class HighlightBlacklistUser;

class Application
{
    std::vector<std::unique_ptr<Singleton>> singletons_;
    int argc_;
    char **argv_;

public:
    static Application *instance;

    Application(Settings &settings, Paths &paths);

    void initialize(Settings &settings, Paths &paths);
    void load();
    void save();

    int run(QApplication &qtApp);

    friend void test();

    // clang-format off
    SignalVector<HighlightPhrase>        &highlightedMessages;
    SignalVector<HighlightPhrase>        &highlightedUsers;
    SignalVector<HighlightBlacklistUser> &blacklistedUsers;
    // clang-format on

    bool isHighlightedUser(const QString &username);
    bool isBlacklistedUser(const QString &username);

    Theme *const themes{};
    Fonts *const fonts{};
    Emotes *const emotes{};
    WindowManager *const windows{};
    Toasts *const toasts{};

    AccountController *const accounts{};
    CommandController *const commands{};
    NotificationController *const notifications{};
    MutedChannelController *const pings{};
    IgnoreController *const ignores{};
    TaggedUsersController *const taggedUsers{};
    ModerationActions *const moderationActions{};
    TwitchIrcServer *const twitch2{};
    ChatterinoBadges *const chatterinoBadges{};

    /*[[deprecated]]*/ Logging *const logging{};

    /// Provider-specific
    struct {
        /*[[deprecated("use twitch2 instead")]]*/ TwitchIrcServer *server{};
        /*[[deprecated("use twitch2->pubsub instead")]]*/ PubSub *pubsub{};
    } twitch;

private:
    void addSingleton(Singleton *singleton);
    void initPubsub();
    void initNm(Paths &paths);

    template <typename T,
              typename = std::enable_if_t<std::is_base_of<Singleton, T>::value>>
    T &emplace()
    {
        auto t = new T;
        this->singletons_.push_back(std::unique_ptr<T>(t));
        return *t;
    }

    NativeMessagingServer nmServer{};
};

Application *getApp();

}  // namespace chatterino
