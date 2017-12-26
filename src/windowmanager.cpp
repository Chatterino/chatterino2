#include "windowmanager.hpp"
#include "appdatapath.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"

#include <QDebug>
#include <QStandardPaths>
#include <boost/foreach.hpp>

namespace chatterino {
WindowManager *WindowManager::instance = nullptr;

WindowManager::WindowManager(ChannelManager &_channelManager, ColorScheme &_colorScheme)
    : channelManager(_channelManager)
    , colorScheme(_colorScheme)
{
    WindowManager::instance = this;
}

void WindowManager::initMainWindow()
{
    this->selectedWindow = this->mainWindow =
        new widgets::Window("main", this->channelManager, this->colorScheme, true);
}

static const std::string &getSettingsPath()
{
    static std::string path = (Path::getAppdataPath() + "uilayout.json").toStdString();

    return path;
}

void WindowManager::layoutVisibleChatWidgets(Channel *channel)
{
    this->layout();
}

void WindowManager::repaintVisibleChatWidgets(Channel *channel)
{
    if (this->mainWindow != nullptr) {
        this->mainWindow->repaintVisibleChatWidgets(channel);
    }
}

void WindowManager::repaintGifEmotes()
{
    this->repaintGifs();
}

// void WindowManager::updateAll()
//{
//    if (this->mainWindow != nullptr) {
//        this->mainWindow->update();
//    }
//}

widgets::Window &WindowManager::getMainWindow()
{
    return *this->mainWindow;
}

widgets::Window &WindowManager::getSelectedWindow()
{
    return *this->selectedWindow;
}

widgets::Window &WindowManager::createWindow()
{
    auto *window = new widgets::Window("external", this->channelManager, this->colorScheme, false);
    window->getNotebook().addNewPage();

    this->windows.push_back(window);

    return *window;
}

int WindowManager::windowCount()
{
    return this->windows.size();
}

widgets::Window *WindowManager::windowAt(int index)
{
    if (index < 0 || (size_t)index >= this->windows.size()) {
        return nullptr;
    }
    qDebug() << "getting window at bad index" << index;

    return this->windows.at(index);
}

void WindowManager::save()
{
    assert(this->mainWindow);

    this->mainWindow->save();

    for (widgets::Window *window : this->windows) {
        window->save();
    }
}

}  // namespace chatterino
