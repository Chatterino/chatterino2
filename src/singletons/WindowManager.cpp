#include "WindowManager.hpp"

#include "Application.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Log.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Theme.hpp"
#include "util/Clamp.hpp"
#include "widgets/AccountSwitchPopupWidget.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <QDebug>

#define SETTINGS_FILENAME "/window-layout.json"

namespace chatterino {

using SplitNode = SplitContainer::Node;
using SplitDirection = SplitContainer::Direction;

const int WindowManager::uiScaleMin = -5;
const int WindowManager::uiScaleMax = 10;

void WindowManager::showSettingsDialog()
{
    QTimer::singleShot(80, [] { SettingsDialog::showDialog(); });
}

void WindowManager::showAccountSelectPopup(QPoint point)
{
    //    static QWidget *lastFocusedWidget = nullptr;
    static AccountSwitchPopupWidget *w = new AccountSwitchPopupWidget();

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

    auto settings = getSettings();

    this->wordFlagsListener_.addSetting(settings->showTimestamps);
    this->wordFlagsListener_.addSetting(settings->showBadges);
    this->wordFlagsListener_.addSetting(settings->enableBttvEmotes);
    this->wordFlagsListener_.addSetting(settings->enableEmojis);
    this->wordFlagsListener_.addSetting(settings->enableFfzEmotes);
    this->wordFlagsListener_.addSetting(settings->enableTwitchEmotes);
    this->wordFlagsListener_.addSetting(settings->enableUsernameBold);
    this->wordFlagsListener_.addSetting(settings->enableLowercaseLink);
    this->wordFlagsListener_.cb = [this](auto) {
        this->updateWordTypeMask();  //
    };
}

MessageElement::Flags WindowManager::getWordFlags()
{
    return this->wordFlags_;
}

void WindowManager::updateWordTypeMask()
{
    using MEF = MessageElement::Flags;
    auto settings = getSettings();

    // text
    auto flags = MEF::Text | MEF::Text;

    // timestamp
    if (settings->showTimestamps) {
        flags |= MEF::Timestamp;
    }

    // emotes
    flags |= settings->enableTwitchEmotes ? MEF::TwitchEmoteImage : MEF::TwitchEmoteText;
    flags |= settings->enableFfzEmotes ? MEF::FfzEmoteImage : MEF::FfzEmoteText;
    flags |= settings->enableBttvEmotes ? MEF::BttvEmoteImage : MEF::BttvEmoteText;
    flags |= settings->enableEmojis ? MEF::EmojiImage : MEF::EmojiText;

    // bits
    flags |= MEF::BitsAmount;
    flags |= settings->enableGifAnimations ? MEF::BitsAnimated : MEF::BitsStatic;

    // badges
    flags |= settings->showBadges ? MEF::Badges : MEF::None;

    // username
    flags |= MEF::Username;

    // misc
    flags |= MEF::AlwaysShow;
    flags |= MEF::Collapsed;
    flags |= settings->enableUsernameBold ? MEF::BoldUsername : MEF::NonBoldUsername;
    flags |= settings->enableLowercaseLink ? MEF::LowercaseLink : MEF::OriginalLink;

    // update flags
    MessageElement::Flags newFlags = static_cast<MessageElement::Flags>(flags);

    if (newFlags != this->wordFlags_) {
        this->wordFlags_ = newFlags;

        this->wordFlagsChanged.invoke();
    }
}

void WindowManager::layoutChannelViews(Channel *channel)
{
    this->layout.invoke(channel);
}

void WindowManager::forceLayoutChannelViews()
{
    this->incGeneration();
    this->layoutChannelViews(nullptr);
}

void WindowManager::repaintVisibleChatWidgets(Channel *channel)
{
    if (this->mainWindow_ != nullptr) {
        this->mainWindow_->repaintVisibleChatWidgets(channel);
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

Window &WindowManager::getMainWindow()
{
    assertInGuiThread();

    return *this->mainWindow_;
}

Window &WindowManager::getSelectedWindow()
{
    assertInGuiThread();

    return *this->selectedWindow_;
}

Window &WindowManager::createWindow(Window::Type type)
{
    assertInGuiThread();

    auto *window = new Window(type);
    this->windows_.push_back(window);
    window->show();

    if (type != Window::Type::Main) {
        window->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(window, &QWidget::destroyed, [this, window] {
            for (auto it = this->windows_.begin(); it != this->windows_.end(); it++) {
                if (*it == window) {
                    this->windows_.erase(it);
                    break;
                }
            }
        });
    }

    return *window;
}

int WindowManager::windowCount()
{
    return this->windows_.size();
}

Window *WindowManager::windowAt(int index)
{
    assertInGuiThread();

    if (index < 0 || (size_t)index >= this->windows_.size()) {
        return nullptr;
    }
    Log("getting window at bad index {}", index);

    return this->windows_.at(index);
}

void WindowManager::initialize(Application &app)
{
    assertInGuiThread();

    app.themes->repaintVisibleChatWidgets_.connect([this] { this->repaintVisibleChatWidgets(); });

    assert(!this->initialized_);

    // load file
    QString settingsPath = getPaths()->settingsDirectory + SETTINGS_FILENAME;
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
        Window::Type type = type_val == "main" ? Window::Type::Main : Window::Type::Popup;

        if (type == Window::Type::Main && mainWindow_ != nullptr) {
            type = Window::Type::Popup;
        }

        Window &window = createWindow(type);

        if (type == Window::Type::Main) {
            mainWindow_ = &window;
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
            SplitContainer *page = window.getNotebook().addPage(false);

            QJsonObject tab_obj = tab_val.toObject();

            // set custom title
            QJsonValue title_val = tab_obj.value("title");
            if (title_val.isString()) {
                page->getTab()->setCustomTitle(title_val.toString());
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
                    Split *split = new Split(page);

                    QJsonObject split_obj = split_val.toObject();
                    split->setChannel(decodeChannel(split_obj));

                    page->appendSplit(split);
                }
                colNr++;
            }
        }
    }

    if (mainWindow_ == nullptr) {
        mainWindow_ = &createWindow(Window::Type::Main);
        mainWindow_->getNotebook().addPage(true);
    }

    auto settings = getSettings();

    settings->timestampFormat.connect([this](auto, auto) {
        auto app = getApp();
        this->layoutChannelViews();
    });

    settings->emoteScale.connect([this](auto, auto) { this->forceLayoutChannelViews(); });

    settings->timestampFormat.connect([this](auto, auto) { this->forceLayoutChannelViews(); });
    settings->alternateMessageBackground.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });
    settings->separateMessages.connect([this](auto, auto) { this->forceLayoutChannelViews(); });
    settings->collpseMessagesMinLines.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });

    this->initialized_ = true;
}

void WindowManager::save()
{
    assertInGuiThread();
    auto app = getApp();

    QJsonDocument document;

    // "serialize"
    QJsonArray window_arr;
    for (Window *window : this->windows_) {
        QJsonObject window_obj;

        // window type
        switch (window->getType()) {
            case Window::Type::Main:
                window_obj.insert("type", "main");
                break;

            case Window::Type::Popup:
                window_obj.insert("type", "popup");
                break;

            case Window::Type::Attached:;
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
            SplitContainer *tab =
                dynamic_cast<SplitContainer *>(window->getNotebook().getPageAt(tab_i));
            assert(tab != nullptr);

            // custom tab title
            if (tab->getTab()->hasCustomTitle()) {
                tab_obj.insert("title", tab->getTab()->getCustomTitle());
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
    QString settingsPath = app->paths->settingsDirectory + SETTINGS_FILENAME;
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
    assertInGuiThread();

    switch (channel.getType()) {
        case Channel::Type::Twitch: {
            obj.insert("type", "twitch");
            obj.insert("name", channel.get()->name);
        } break;
        case Channel::Type::TwitchMentions: {
            obj.insert("type", "mentions");
        } break;
        case Channel::Type::TwitchWatching: {
            obj.insert("type", "watching");
        } break;
        case Channel::Type::TwitchWhispers: {
            obj.insert("type", "whispers");
        } break;
    }
}

IndirectChannel WindowManager::decodeChannel(const QJsonObject &obj)
{
    assertInGuiThread();

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
    assertInGuiThread();

    for (Window *window : windows_) {
        window->close();
    }
}

int WindowManager::getGeneration() const
{
    return this->generation_;
}

void WindowManager::incGeneration()
{
    this->generation_++;
}

int WindowManager::clampUiScale(int scale)
{
    return clamp(scale, uiScaleMin, uiScaleMax);
}

float WindowManager::getUiScaleValue()
{
    return getUiScaleValue(getApp()->settings->uiScale.getValue());
}

float WindowManager::getUiScaleValue(int scale)
{
    switch (clampUiScale(scale)) {
        case -5:
            return 0.5f;
        case -4:
            return 0.6f;
        case -3:
            return 0.7f;
        case -2:
            return 0.8f;
        case -1:
            return 0.9f;
        case 0:
            return 1;
        case 1:
            return 1.2f;
        case 2:
            return 1.4f;
        case 3:
            return 1.6f;
        case 4:
            return 1.6f;
        case 5:
            return 2;
        case 6:
            return 2.33f;
        case 7:
            return 2.66f;
        case 8:
            return 3;
        case 9:
            return 3.5f;
        case 10:
            return 4;
        default:
            assert(false);
    }
}

}  // namespace chatterino
