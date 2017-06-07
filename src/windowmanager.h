#pragma once

#include "widgets/mainwindow.h"

#include <mutex>

namespace chatterino {

class WindowManager
{
public:
    static WindowManager &getInstance()
    {
        return instance;
    }

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    void updateAll();

    widgets::MainWindow &getMainWindow();

    void load();
    void save();

private:
    static WindowManager instance;

    WindowManager();

    std::mutex _windowMutex;
    widgets::MainWindow *_mainWindow;
};

}  // namespace chatterino
