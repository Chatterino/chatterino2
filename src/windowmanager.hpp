#pragma once

#include "widgets/window.hpp"

#include <mutex>

namespace chatterino {

class ChannelManager;
class ColorScheme;
class CompletionManager;

class WindowManager
{
public:
    explicit WindowManager(ChannelManager &_channelManager, ColorScheme &_colorScheme,
                           CompletionManager &_completionManager);

    static WindowManager *instance;

    ChannelManager &channelManager;
    ColorScheme &colorScheme;
    CompletionManager &completionManager;

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    // void updateAll();

    widgets::Window &getMainWindow();
    widgets::Window &getCurrentWindow();

    widgets::Window &createWindow();

    void load();
    void save();

    boost::signals2::signal<void()> repaintGifs;
    boost::signals2::signal<void()> layout;

private:
    std::mutex windowMutex;
    std::vector<widgets::Window *> windows;

    // TODO(pajlada): Store as a value instead of a pointer
    widgets::Window *mainWindow = nullptr;
};

}  // namespace chatterino
