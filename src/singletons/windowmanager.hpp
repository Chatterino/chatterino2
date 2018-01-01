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

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    void initMainWindow();
    // void updateAll();

    widgets::Window &getMainWindow();
    widgets::Window &getSelectedWindow();
    widgets::Window &createWindow();

    int windowCount();
    widgets::Window *windowAt(int index);

    void save();

    boost::signals2::signal<void()> repaintGifs;
    boost::signals2::signal<void(Channel *)> layout;

private:
    ThemeManager &themeManager;

    std::vector<widgets::Window *> windows;

    widgets::Window *mainWindow = nullptr;
    widgets::Window *selectedWindow = nullptr;
};

}  // namespace chatterino
}
