#pragma once

#include "widgets/mainwindow.hpp"

#include <mutex>

namespace chatterino {

class ChannelManager;

class WindowManager
{
public:
    explicit WindowManager(ChannelManager &_channelManager);

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    void updateAll();

    widgets::MainWindow &getMainWindow();

    void load();
    void save();

private:
    ChannelManager &channelManager;

    std::mutex windowMutex;

    // TODO(pajlada): Store as a value instead of a pointer
    widgets::MainWindow *mainWindow = nullptr;
};

}  // namespace chatterino
