#include "windowmanager.hpp"
#include "debug/log.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/accountswitchpopupwidget.hpp"
#include "widgets/settingsdialog.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

#define SETTINGS_FILENAME "/layout.json"

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

widgets::Window &WindowManager::createWindow(widgets::Window::WindowType type)
{
    auto *window = new widgets::Window(this->themeManager, type);
    this->windows.push_back(window);
    window->show();

    if (type != widgets::Window::Main) {
        window->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(window, &QWidget::destroyed, [this, window] {
            for (auto it = this->windows.begin(); it != this->windows.end(); it++) {
                if (*it == window) {
                    this->windows.erase(it);
                    break;
                }
            }
        });
    }

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

void WindowManager::initialize()
{
    assert(!this->initialized);

    // load file
    QString settingsPath = PathManager::getInstance().settingsFolderPath + SETTINGS_FILENAME;
    QFile file(settingsPath);
    file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    QJsonDocument document = QJsonDocument::fromJson(data);
    QJsonArray windows_arr = document.object().value("windows").toArray();

    // "deserialize"
    for (QJsonValue window_val : windows_arr) {
        QJsonObject window_obj = window_val.toObject();

        // get type
        QString type_val = window_obj.value("type").toString();
        widgets::Window::WindowType type =
            type_val == "main" ? widgets::Window::Main : widgets::Window::Popup;

        if (type == widgets::Window::Main && mainWindow != nullptr) {
            type = widgets::Window::Popup;
        }

        widgets::Window &window = createWindow(type);

        if (type == widgets::Window::Main) {
            mainWindow = &window;
        }

        // get geometry
        {
            int x = window_obj.value("x").toInt(-1);
            int y = window_obj.value("y").toInt(-1);
            int width = window_obj.value("width").toInt(-1);
            int height = window_obj.value("height").toInt(-1);

            if (x != -1 && y != -1 && width != -1 && height != -1) {
                window.setGeometry(x, y, width, height);
            }
        }

        // load tabs
        QJsonArray tabs = window_obj.value("tabs").toArray();
        for (QJsonValue tab_val : tabs) {
            widgets::SplitContainer *tab = window.getNotebook().addNewPage();

            QJsonObject tab_obj = tab_val.toObject();

            // set custom title
            QJsonValue title_val = tab_obj.value("title");
            if (title_val.isString()) {
                tab->getTab()->setTitle(title_val.toString());
                tab->getTab()->useDefaultTitle = false;
            }

            // selected
            if (tab_obj.value("selected").toBool(false)) {
                window.getNotebook().select(tab);
            }

            // load splits
            int colNr = 0;
            for (QJsonValue column_val : tab_obj.value("splits").toArray()) {
                for (QJsonValue split_val : column_val.toArray()) {
                    widgets::Split *split = new widgets::Split(tab);

                    QJsonObject split_obj = split_val.toObject();
                    QJsonValue channelName_val = split_obj.value("channelName");
                    if (channelName_val.isString()) {
                        split->setChannel(providers::twitch::TwitchServer::getInstance().addChannel(
                            channelName_val.toString()));
                    }

                    tab->addToLayout(split, std::make_pair(colNr, -1));
                }
                colNr++;
            }
        }
    }

    if (mainWindow == nullptr) {
        mainWindow = &createWindow(widgets::Window::Main);
        mainWindow->getNotebook().addNewPage(true);
    }

    this->initialized = true;
}

void WindowManager::save()
{
    QJsonDocument document;

    // "serialize"
    QJsonArray window_arr;
    for (widgets::Window *window : this->windows) {
        QJsonObject window_obj;

        // window type
        switch (window->getType()) {
            case widgets::Window::Main:
                window_obj.insert("type", "main");
                break;
            case widgets::Window::Popup:
                window_obj.insert("type", "popup");
                break;
        }

        // window geometry
        window_obj.insert("x", window->x());
        window_obj.insert("y", window->y());
        window_obj.insert("width", window->width());
        window_obj.insert("height", window->height());

        // window tabs
        QJsonArray tabs_arr;

        for (int tab_i = 0; tab_i < window->getNotebook().tabCount(); tab_i++) {
            QJsonObject tab_obj;
            widgets::SplitContainer *tab = window->getNotebook().tabAt(tab_i);

            // custom tab title
            if (!tab->getTab()->useDefaultTitle) {
                tab_obj.insert("title", tab->getTab()->getTitle());
            }

            // selected
            if (window->getNotebook().getSelectedPage() == tab) {
                tab_obj.insert("selected", true);
            }

            // splits
            QJsonArray columns_arr;
            std::vector<std::vector<widgets::Split *>> columns = tab->getColumns();

            for (std::vector<widgets::Split *> &cells : columns) {
                QJsonArray cells_arr;

                for (widgets::Split *cell : cells) {
                    QJsonObject cell_obj;
                    cell_obj.insert("channelName", cell->getChannel()->name);

                    cells_arr.append(cell_obj);
                }
                columns_arr.append(cells_arr);
            }

            tab_obj.insert("splits", columns_arr);
            tabs_arr.append(tab_obj);
        }

        window_obj.insert("tabs", tabs_arr);
        window_arr.append(window_obj);
    }

    QJsonObject obj;
    obj.insert("windows", window_arr);
    document.setObject(obj);

    // save file
    QString settingsPath = PathManager::getInstance().settingsFolderPath + SETTINGS_FILENAME;
    QFile file(settingsPath);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    file.write(document.toJson());
    file.flush();
}

void WindowManager::closeAll()
{
    for (widgets::Window *window : windows) {
        window->close();
    }
}

}  // namespace singletons
}  // namespace chatterino
