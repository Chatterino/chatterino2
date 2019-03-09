#include "Application.hpp"

#include <stdlib.h>
#include <QApplication>
#include <QVector>

#include "net/NetworkManager.hpp"
#include "twitch/TwitchProvider.hpp"
#include "ui/Window.hpp"

#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "controllers/ignores/IgnoreController.hpp"
#include "controllers/moderationactions/ModerationActions.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "controllers/taggedusers/TaggedUsersController.hpp"
#include "messages/MessageBuilder.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/chatterino/ChatterinoBadges.hpp"
#include "providers/ffz/FfzEmotes.hpp"
#include "providers/twitch/PubsubClient.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
//#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Logging.hpp"
#include "singletons/NativeMessaging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Toasts.hpp"
//#include "util/IsBigEndian.hpp"
//#include "util/Log.hpp"
//#include "util/PostToThread.hpp"
#include "util/CombinePath.hpp"

namespace chatterino
{
    constexpr const char* WINDOW_LAYOUT_FILENAME = "window-layout.json";

    class PrivateApplication
    {
    public:
        // windows
        ui::Window* mainWindow{};
        QVector<ui::Window*> windows;

        // providers
        QVector<Provider*> providers;

        // misc
        QObject object;
        // QApplication& qtApp;
    };

    inline void deserializeWindows(Application& app, const QJsonObject& root)
    {
        // deserialize
        for (QJsonValue window_val : root.value("windows").toArray())
        {
            auto window_obj = window_val.toObject();

            auto type = window_obj.value("type").toString() == "main"
                            ? ui::WindowType::Main
                            : ui::WindowType::Popup;

            auto window = app.addWindow(type);
            window->deserialize(window_obj);
        }
    }

    inline QJsonObject serializeWindows([[maybe_unused]] Application& app)
    {
        assert(false);
        return {};
    }

    Application::Application()
        // leaking (compatability)
        : toasts(new Toasts())
        , emotes(new Emotes())
        , accounts(new AccountController())
        //, commands(new CommandController())
        , highlights(new HighlightController())
        , notifications(new NotificationController())
        , ignores(new IgnoreController())
        , taggedUsers(new TaggedUsersController())
        , moderationActions(new ModerationActions())
        , chatterinoBadges(new ChatterinoBadges())
        , logging(new Logging())

        , this_(new PrivateApplication())
    {
        appInst__ = this;

        this_->providers.append(new TwitchProvider());

        // load window layout
        {
            QFile file(combinePath(
                getPaths()->settingsDirectory, WINDOW_LAYOUT_FILENAME));
            file.open(QIODevice::ReadOnly);
            deserializeWindows(
                *this, QJsonDocument::fromJson(file.readAll()).object());
        }

        NetworkManager::init();
    }

    Application::~Application()
    {
        _exit(0);

        delete this_;
    }

    void Application::run(QApplication& qtApp)
    {
        // make sure that there is a main window
        if (!this_->mainWindow)
            addWindow(ui::WindowType::Main);

        assert(this_->mainWindow);

        // run the main event loop
        qtApp.exec();
    }

    ui::Window* Application::addWindow(const ui::WindowType& type_)
    {
        // there can only be one main window
        auto type = type_ == ui::WindowType::Main && this_->mainWindow
                        ? ui::WindowType::Popup
                        : type_;

        // make new window
        auto window = new ui::Window(*this, type);

        // add to list
        this_->windows.append(window);

        if (type == ui::WindowType::Main)
            this_->mainWindow = window;

        // remove from list when destroyed
        QObject::connect(window, &QWidget::destroyed, &this_->object,
            [=]() { this_->windows.removeOne(window); });

        // show
        window->show();

        return window;
    }

    const QVector<Provider*>& Application::providers()
    {
        return this_->providers;
    }

    void Application::alert()
    {
        int duration = getSettings()->longAlerts ? 2500 : 0;

        if (this_->mainWindow)
            QApplication::alert(this_->mainWindow->window(), duration);
    }

    void Application::initialize(Settings& settings, Paths& paths)
    {
        // compatability
        this->toasts->initialize(settings, paths);
        this->emotes->initialize(settings, paths);

        this->accounts->initialize(settings, paths);
        // this->commands->initialize(settings, paths);
        this->highlights->initialize(settings, paths);
        this->notifications->initialize(settings, paths);
        this->ignores->initialize(settings, paths);
        this->taggedUsers->initialize(settings, paths);
        this->moderationActions->initialize(settings, paths);
        this->chatterinoBadges->initialize(settings, paths);

        this->logging->initialize(settings, paths);
    }

    Application* getApp()
    {
        assert(appInst__);

        return appInst__;
    }
}  // namespace chatterino
