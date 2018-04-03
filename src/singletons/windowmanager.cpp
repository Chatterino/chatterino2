#include "windowmanager.hpp"
#include "debug/log.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/accountswitchpopupwidget.hpp"
#include "widgets/settingsdialog.hpp"

#include <QDebug>

namespace chatterino {
namespace singletons {

WindowManager &WindowManager::getInstance()
{
    static WindowManager instance(ThemeManager::getInstance());
    return instance;
}

void WindowManager::showSettingsDialog()
{
    QTimer::singleShot(80, [] { widgets::SettingsDialog::showDialog(); });
}

void WindowManager::showAccountSelectPopup(QPoint point)
{
    //    static QWidget *lastFocusedWidget = nullptr;
    static widgets::AccountSwitchPopupWidget *w = new widgets::AccountSwitchPopupWidget();

    if (w->hasFocus()) {
        w->hide();
        //            if (lastFocusedWidget) {
        //                lastFocusedWidget->setFocus();
        //            }
        return;
    }

    //    lastFocusedWidget = this->focusWidget();

    w->refresh();

    QPoint buttonPos = point;
    w->move(buttonPos.x(), buttonPos.y());

    w->show();
    w->setFocus();
}

WindowManager::WindowManager(ThemeManager &_themeManager)
    : themeManager(_themeManager)
{
    _themeManager.repaintVisibleChatWidgets.connect([this] { this->repaintVisibleChatWidgets(); });
}

void WindowManager::initMainWindow()
{
    this->selectedWindow = this->mainWindow = new widgets::Window("main", this->themeManager, true);
}

void WindowManager::layoutVisibleChatWidgets(Channel *channel)
{
    this->layout.invoke(channel);
}

void WindowManager::repaintVisibleChatWidgets(Channel *channel)
{
    if (this->mainWindow != nullptr) {
        this->mainWindow->repaintVisibleChatWidgets(channel);
    }
}

void WindowManager::repaintGifEmotes()
{
    this->repaintGifs.invoke();
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
    auto *window = new widgets::Window("external", this->themeManager, false);
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
    debug::Log("getting window at bad index {}", index);

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

}  // namespace singletons
}  // namespace chatterino
