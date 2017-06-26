#pragma once

#include "widgets/mainwindow.hpp"

#include <mutex>

namespace chatterino {

class ChannelManager;
class ColorScheme;

class WindowManager
{
public:
    explicit WindowManager(ChannelManager &_channelManager, ColorScheme &_colorScheme);

    ChannelManager &channelManager;
    ColorScheme &colorScheme;

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    void updateAll();

    widgets::MainWindow &getMainWindow();

    void load();
    void save();

private:
    std::mutex windowMutex;

    // TODO(pajlada): Store as a value instead of a pointer
    widgets::MainWindow *mainWindow = nullptr;
};

}  // namespace chatterino
