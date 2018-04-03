#pragma once

#include "widgets/window.hpp"

namespace chatterino {
namespace singletons {

class ThemeManager;

class WindowManager
{
    explicit WindowManager(ThemeManager &_themeManager);

public:
    static WindowManager &getInstance();

    void showSettingsDialog();
    void showAccountSelectPopup(QPoint point);

    void initMainWindow();
    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    // void updateAll();

    widgets::Window &getMainWindow();
    widgets::Window &getSelectedWindow();
    widgets::Window &createWindow();

    int windowCount();
    widgets::Window *windowAt(int index);

    void save();

    pajlada::Signals::NoArgSignal repaintGifs;
    pajlada::Signals::Signal<Channel *> layout;

private:
    ThemeManager &themeManager;

    std::vector<widgets::Window *> windows;

    widgets::Window *mainWindow = nullptr;
    widgets::Window *selectedWindow = nullptr;
};

}  // namespace singletons
}  // namespace chatterino
