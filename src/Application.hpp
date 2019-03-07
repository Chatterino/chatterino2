#pragma once

class QApplication;
template <typename T>
class QVector;
namespace chatterino::ui
{
    class Window;
    enum class WindowType;
}  // namespace chatterino::ui

namespace chatterino
{
    class Provider;
    class PrivateApplication;

    // compatability
    class TwitchServer;
    class PubSub;

    class CommandController;
    class HighlightController;
    class IgnoreController;
    class TaggedUsersController;
    class AccountController;
    class ModerationActions;
    class NotificationController;

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

    class Application
    {
    public:
        Application();
        ~Application();

        // execution
        void run(QApplication& qtApp);
        ui::Window* addWindow(const ui::WindowType& type);

        // providers
        const QVector<Provider*>& providers();

        // compatability
        Emotes* const emotes{};
        WindowManager* const windows{};
        Toasts* const toasts{};

        AccountController* const accounts{};
        CommandController* const commands{};
        HighlightController* const highlights{};
        NotificationController* const notifications{};
        IgnoreController* const ignores{};
        TaggedUsersController* const taggedUsers{};
        ModerationActions* const moderationActions{};
        ChatterinoBadges* const chatterinoBadges{};

        Logging* const logging{};

    private:
        PrivateApplication* this_{};  // delete this manually
    };
}  // namespace chatterino
