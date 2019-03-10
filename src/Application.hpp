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
    class Settings;
    class Paths;

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
    class Logging;
    class Paths;
    class AccountManager;
    class Emotes;
    class Settings;
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
        Provider* provider(const QString& id);

        // misc
        void alert();

        // compatability
        void initialize(Settings& settings, Paths& paths);
        Toasts* const toasts{};
        Emotes* const emotes{};

        AccountController* const accounts{};
        // CommandController* const commands{};
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

    [[deprecated("use dependency injection")]] inline Application* appInst__{};
    [[deprecated("use dependency injection")]] Application* getApp();
}  // namespace chatterino
