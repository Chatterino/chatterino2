#pragma once

#include "widgets/window.hpp"

namespace chatterino {

class ChannelManager;
class ColorScheme;
class CompletionManager;

class WindowManager
{
public:
    explicit WindowManager(ChannelManager &_channelManager, ColorScheme &_colorScheme);

    static WindowManager *instance;

    ChannelManager &channelManager;
    ColorScheme &colorScheme;

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

    void load();
    void save();

    boost::signals2::signal<void()> repaintGifs;
    boost::signals2::signal<void()> layout;

private:
    std::vector<widgets::Window *> windows;

    widgets::Window *mainWindow = nullptr;
    widgets::Window *selectedWindow = nullptr;
};

}  // namespace chatterino
