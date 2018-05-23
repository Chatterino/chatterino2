#include "windowmanager.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "providers/twitch/twitchserver.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/thememanager.hpp"
#include "util/assertinguithread.hpp"
#include "widgets/accountswitchpopupwidget.hpp"
#include "widgets/settingsdialog.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

#define SETTINGS_FILENAME "/layout.json"

namespace chatterino {
namespace singletons {

using SplitNode = widgets::SplitContainer::Node;
using SplitDirection = widgets::SplitContainer::Direction;

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

WindowManager::WindowManager()
{
    qDebug() << "init WindowManager";
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
    util::assertInGuiThread();

    return *this->mainWindow;
}

widgets::Window &WindowManager::getSelectedWindow()
{
    util::assertInGuiThread();

    return *this->selectedWindow;
}

widgets::Window &WindowManager::createWindow(widgets::Window::WindowType type)
{
    util::assertInGuiThread();

    auto *window = new widgets::Window(type);
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
    util::assertInGuiThread();

    if (index < 0 || (size_t)index >= this->windows.size()) {
        return nullptr;
    }
    debug::Log("getting window at bad index {}", index);

    return this->windows.at(index);
}

void WindowManager::initialize()
{
    util::assertInGuiThread();

    auto app = getApp();
    app->themes->repaintVisibleChatWidgets.connect([this] { this->repaintVisibleChatWidgets(); });

    assert(!this->initialized);

    // load file
    QString settingsPath = app->paths->settingsFolderPath + SETTINGS_FILENAME;
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
            widgets::SplitContainer *page = window.getNotebook().addPage(false);

            QJsonObject tab_obj = tab_val.toObject();

            // set custom title
            QJsonValue title_val = tab_obj.value("title");
            if (title_val.isString()) {
                page->getTab()->setTitle(title_val.toString());
                page->getTab()->useDefaultTitle = false;
            }

            // selected
            if (tab_obj.value("selected").toBool(false)) {
                window.getNotebook().select(page);
            }

            // load splits
            QJsonObject splitRoot = tab_obj.value("splits2").toObject();

            if (!splitRoot.isEmpty()) {
                page->decodeFromJson(splitRoot);

                continue;
            }

            // fallback load splits (old)
            int colNr = 0;
            for (QJsonValue column_val : tab_obj.value("splits").toArray()) {
                for (QJsonValue split_val : column_val.toArray()) {
                    widgets::Split *split = new widgets::Split(page);

                    QJsonObject split_obj = split_val.toObject();
                    split->setChannel(decodeChannel(split_obj));

                    page->appendSplit(split);
                }
                colNr++;
            }
        }
    }

    if (mainWindow == nullptr) {
        mainWindow = &createWindow(widgets::Window::Main);
        mainWindow->getNotebook().addPage(true);
    }

    this->initialized = true;
}

void WindowManager::save()
{
    util::assertInGuiThread();
    auto app = getApp();

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

        for (int tab_i = 0; tab_i < window->getNotebook().getPageCount(); tab_i++) {
            QJsonObject tab_obj;
            widgets::SplitContainer *tab =
                dynamic_cast<widgets::SplitContainer *>(window->getNotebook().getPageAt(tab_i));
            assert(tab != nullptr);

            // custom tab title
            if (!tab->getTab()->useDefaultTitle) {
                tab_obj.insert("title", tab->getTab()->getTitle());
            }

            // selected
            if (window->getNotebook().getSelectedPage() == tab) {
                tab_obj.insert("selected", true);
            }

            // splits
            QJsonObject splits;

            this->encodeNodeRecusively(tab->getBaseNode(), splits);

            tab_obj.insert("splits2", splits);
            tabs_arr.append(tab_obj);
        }

        window_obj.insert("tabs", tabs_arr);
        window_arr.append(window_obj);
    }

    QJsonObject obj;
    obj.insert("windows", window_arr);
    document.setObject(obj);

    // save file
    QString settingsPath = app->paths->settingsFolderPath + SETTINGS_FILENAME;
    QFile file(settingsPath);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QJsonDocument::JsonFormat format =
#ifdef _DEBUG
        QJsonDocument::JsonFormat::Compact
#else
        (QJsonDocument::JsonFormat)0
#endif
        ;

    file.write(document.toJson(format));
    file.flush();
}

void WindowManager::encodeNodeRecusively(SplitNode *node, QJsonObject &obj)
{
    switch (node->getType()) {
        case SplitNode::_Split: {
            obj.insert("type", "split");
            QJsonObject split;
            encodeChannel(node->getSplit()->getIndirectChannel(), split);
            obj.insert("data", split);
            obj.insert("flexh", node->getHorizontalFlex());
            obj.insert("flexv", node->getVerticalFlex());
        } break;
        case SplitNode::HorizontalContainer:
        case SplitNode::VerticalContainer: {
            obj.insert("type", node->getType() == SplitNode::HorizontalContainer ? "horizontal"
                                                                                 : "vertical");

            QJsonArray items_arr;
            for (const std::unique_ptr<SplitNode> &n : node->getChildren()) {
                QJsonObject subObj;
                this->encodeNodeRecusively(n.get(), subObj);
                items_arr.append(subObj);
            }
            obj.insert("items", items_arr);
        } break;
    }
}

void WindowManager::encodeChannel(IndirectChannel channel, QJsonObject &obj)
{
    util::assertInGuiThread();

    switch (channel.getType()) {
        case Channel::Twitch: {
            obj.insert("type", "twitch");
            obj.insert("name", channel.get()->name);
        } break;
        case Channel::TwitchMentions: {
            obj.insert("type", "mentions");
        } break;
        case Channel::TwitchWatching: {
            obj.insert("type", "watching");
        } break;
        case Channel::TwitchWhispers: {
            obj.insert("type", "whispers");
        } break;
    }
}

IndirectChannel WindowManager::decodeChannel(const QJsonObject &obj)
{
    util::assertInGuiThread();

    auto app = getApp();

    QString type = obj.value("type").toString();
    if (type == "twitch") {
        return app->twitch.server->getOrAddChannel(obj.value("name").toString());
    } else if (type == "mentions") {
        return app->twitch.server->mentionsChannel;
    } else if (type == "watching") {
        return app->twitch.server->watchingChannel;
    } else if (type == "whispers") {
        return app->twitch.server->whispersChannel;
    }

    return Channel::getEmpty();
}

void WindowManager::closeAll()
{
    util::assertInGuiThread();

    for (widgets::Window *window : windows) {
        window->close();
    }
}

}  // namespace singletons
}  // namespace chatterino
