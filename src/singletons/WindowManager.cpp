#include "singletons/WindowManager.hpp"

#include "Application.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "debug/Log.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/Clamp.hpp"
#include "widgets/AccountSwitchPopupWidget.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QDebug>
#include <QDesktopWidget>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSaveFile>
#include <QScreen>
#include <boost/optional.hpp>

#include <chrono>

#define SETTINGS_FILENAME "/window-layout.json"

namespace chatterino {
namespace {
    QJsonArray loadWindowArray(const QString &settingsPath)
    {
        QFile file(settingsPath);
        file.open(QIODevice::ReadOnly);
        QByteArray data = file.readAll();
        QJsonDocument document = QJsonDocument::fromJson(data);
        QJsonArray windows_arr = document.object().value("windows").toArray();
        return windows_arr;
    }

    boost::optional<bool> &shouldMoveOutOfBoundsWindow()
    {
        static boost::optional<bool> x;
        return x;
    }
}  // namespace

using SplitNode = SplitContainer::Node;
using SplitDirection = SplitContainer::Direction;

void WindowManager::showSettingsDialog(SettingsDialogPreference preference)
{
    QTimer::singleShot(
        80, [preference] { SettingsDialog::showDialog(preference); });
}

void WindowManager::showAccountSelectPopup(QPoint point)
{
    //    static QWidget *lastFocusedWidget = nullptr;
    static AccountSwitchPopupWidget *w = new AccountSwitchPopupWidget();

    if (w->hasFocus())
    {
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
    this->wordFlagsListener_.addSetting(settings->showBadgesGlobalAuthority);
    this->wordFlagsListener_.addSetting(settings->showBadgesChannelAuthority);
    this->wordFlagsListener_.addSetting(settings->showBadgesSubscription);
    this->wordFlagsListener_.addSetting(settings->showBadgesVanity);
    this->wordFlagsListener_.addSetting(settings->showBadgesChatterino);
    this->wordFlagsListener_.addSetting(settings->enableEmoteImages);
    this->wordFlagsListener_.addSetting(settings->boldUsernames);
    this->wordFlagsListener_.addSetting(settings->lowercaseDomains);
    this->wordFlagsListener_.setCB([this] {
        this->updateWordTypeMask();  //
    });

    this->saveTimer = new QTimer;

    this->saveTimer->setSingleShot(true);

    QObject::connect(this->saveTimer, &QTimer::timeout, [] {
        getApp()->windows->save();  //
    });
}

MessageElementFlags WindowManager::getWordFlags()
{
    return this->wordFlags_;
}

void WindowManager::updateWordTypeMask()
{
    using MEF = MessageElementFlag;
    auto settings = getSettings();

    // text
    auto flags = MessageElementFlags(MEF::Text);

    // timestamp
    if (settings->showTimestamps)
    {
        flags.set(MEF::Timestamp);
    }

    // emotes
    if (settings->enableEmoteImages)
    {
        flags.set(MEF::EmoteImages);
    }
    flags.set(MEF::EmoteText);
    flags.set(MEF::EmojiText);

    // bits
    flags.set(MEF::BitsAmount);
    flags.set(settings->animateEmotes ? MEF::BitsAnimated : MEF::BitsStatic);

    // badges
    flags.set(settings->showBadgesGlobalAuthority ? MEF::BadgeGlobalAuthority
                                                  : MEF::None);
    flags.set(settings->showBadgesChannelAuthority ? MEF::BadgeChannelAuthority
                                                   : MEF::None);
    flags.set(settings->showBadgesSubscription ? MEF::BadgeSubscription
                                               : MEF::None);
    flags.set(settings->showBadgesVanity ? MEF::BadgeVanity : MEF::None);
    flags.set(settings->showBadgesChatterino ? MEF::BadgeChatterino
                                             : MEF::None);

    // username
    flags.set(MEF::Username);

    // misc
    flags.set(MEF::AlwaysShow);
    flags.set(MEF::Collapsed);
    flags.set(settings->boldUsernames ? MEF::BoldUsername
                                      : MEF::NonBoldUsername);
    flags.set(settings->lowercaseDomains ? MEF::LowercaseLink
                                         : MEF::OriginalLink);

    // update flags
    MessageElementFlags newFlags = static_cast<MessageElementFlags>(flags);

    if (newFlags != this->wordFlags_)
    {
        this->wordFlags_ = newFlags;

        this->wordFlagsChanged.invoke();
    }
}

void WindowManager::layoutChannelViews(Channel *channel)
{
    this->layoutRequested.invoke(channel);
}

void WindowManager::forceLayoutChannelViews()
{
    this->incGeneration();
    this->layoutChannelViews(nullptr);
}

void WindowManager::repaintVisibleChatWidgets(Channel *channel)
{
    this->layoutRequested.invoke(channel);
}

void WindowManager::repaintGifEmotes()
{
    this->gifRepaintRequested.invoke();
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

Window &WindowManager::createWindow(WindowType type, bool show)
{
    assertInGuiThread();

    auto *window = new Window(type);
    this->windows_.push_back(window);
    if (show)
    {
        window->show();
    }

    if (type != WindowType::Main)
    {
        window->setAttribute(Qt::WA_DeleteOnClose);

        QObject::connect(window, &QWidget::destroyed, [this, window] {
            for (auto it = this->windows_.begin(); it != this->windows_.end();
                 it++)
            {
                if (*it == window)
                {
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

    if (index < 0 || (size_t)index >= this->windows_.size())
    {
        return nullptr;
    }
    log("getting window at bad index {}", index);

    return this->windows_.at(index);
}

void WindowManager::initialize(Settings &settings, Paths &paths)
{
    assertInGuiThread();

    getApp()->themes->repaintVisibleChatWidgets_.connect(
        [this] { this->repaintVisibleChatWidgets(); });

    assert(!this->initialized_);

    // load file
    QString settingsPath = getPaths()->settingsDirectory + SETTINGS_FILENAME;
    QJsonArray windows_arr = loadWindowArray(settingsPath);

    // "deserialize"
    for (QJsonValue window_val : windows_arr)
    {
        QJsonObject window_obj = window_val.toObject();

        // get type
        QString type_val = window_obj.value("type").toString();
        WindowType type =
            type_val == "main" ? WindowType::Main : WindowType::Popup;

        if (type == WindowType::Main && mainWindow_ != nullptr)
        {
            type = WindowType::Popup;
        }

        Window &window = createWindow(type, false);

        if (window_obj.value("state") == "maximized")
        {
            window.setWindowState(Qt::WindowMaximized);
        }
        else if (window_obj.value("state") == "minimized")
        {
            window.setWindowState(Qt::WindowMinimized);
        }

        if (type == WindowType::Main)
        {
            mainWindow_ = &window;
        }

        // get geometry
        {
            int x = window_obj.value("x").toInt(-1);
            int y = window_obj.value("y").toInt(-1);
            int width = window_obj.value("width").toInt(-1);
            int height = window_obj.value("height").toInt(-1);

            QRect geometry{x, y, width, height};

            // out of bounds windows
            auto screens = qApp->screens();
            bool outOfBounds = std::none_of(
                screens.begin(), screens.end(), [&](QScreen *screen) {
                    return screen->availableGeometry().intersects(geometry);
                });

            // ask if move into bounds
            auto &&should = shouldMoveOutOfBoundsWindow();
            if (outOfBounds && !should)
            {
                should =
                    QMessageBox(QMessageBox::Icon::Warning,
                                "Windows out of bounds",
                                "Some windows were detected out of bounds. "
                                "Should they be moved into bounds?",
                                QMessageBox::Yes | QMessageBox::No)
                        .exec() == QMessageBox::Yes;
            }

            if ((!outOfBounds || !should.value()) && x != -1 && y != -1 &&
                width != -1 && height != -1)
            {
                // Have to offset x by one because qt moves the window 1px too
                // far to the left:w

                window.setGeometry(x + 1, y, width, height);
            }
        }

        // load tabs
        QJsonArray tabs = window_obj.value("tabs").toArray();
        for (QJsonValue tab_val : tabs)
        {
            SplitContainer *page = window.getNotebook().addPage(false);

            QJsonObject tab_obj = tab_val.toObject();

            // set custom title
            QJsonValue title_val = tab_obj.value("title");
            if (title_val.isString())
            {
                page->getTab()->setCustomTitle(title_val.toString());
            }

            // selected
            if (tab_obj.value("selected").toBool(false))
            {
                window.getNotebook().select(page);
            }

            // highlighting on new messages
            bool val = tab_obj.value("highlightsEnabled").toBool(true);
            page->getTab()->setHighlightsEnabled(val);

            // load splits
            QJsonObject splitRoot = tab_obj.value("splits2").toObject();

            if (!splitRoot.isEmpty())
            {
                page->decodeFromJson(splitRoot);

                continue;
            }

            // fallback load splits (old)
            int colNr = 0;
            for (QJsonValue column_val : tab_obj.value("splits").toArray())
            {
                for (QJsonValue split_val : column_val.toArray())
                {
                    Split *split = new Split(page);

                    QJsonObject split_obj = split_val.toObject();
                    split->setChannel(decodeChannel(split_obj));

                    page->appendSplit(split);
                }
                colNr++;
            }
        }
        window.show();
    }

    if (mainWindow_ == nullptr)
    {
        mainWindow_ = &createWindow(WindowType::Main);
        mainWindow_->getNotebook().addPage(true);
    }

    settings.timestampFormat.connect(
        [this](auto, auto) { this->layoutChannelViews(); });

    settings.emoteScale.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });

    settings.timestampFormat.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });
    settings.alternateMessages.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });
    settings.separateMessages.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });
    settings.collpseMessagesMinLines.connect(
        [this](auto, auto) { this->forceLayoutChannelViews(); });

    this->initialized_ = true;
}

void WindowManager::save()
{
    log("[WindowManager] Saving");
    assertInGuiThread();
    QJsonDocument document;

    // "serialize"
    QJsonArray window_arr;
    for (Window *window : this->windows_)
    {
        QJsonObject window_obj;

        // window type
        switch (window->getType())
        {
            case WindowType::Main:
                window_obj.insert("type", "main");
                break;

            case WindowType::Popup:
                window_obj.insert("type", "popup");
                break;

            case WindowType::Attached:;
        }

        if (window->isMaximized())
        {
            window_obj.insert("state", "maximized");
        }
        else if (window->isMinimized())
        {
            window_obj.insert("state", "minimized");
        }

        // window geometry
        window_obj.insert("x", window->x());
        window_obj.insert("y", window->y());
        window_obj.insert("width", window->width());
        window_obj.insert("height", window->height());

        // window tabs
        QJsonArray tabs_arr;

        for (int tab_i = 0; tab_i < window->getNotebook().getPageCount();
             tab_i++)
        {
            QJsonObject tab_obj;
            SplitContainer *tab = dynamic_cast<SplitContainer *>(
                window->getNotebook().getPageAt(tab_i));
            assert(tab != nullptr);

            // custom tab title
            if (tab->getTab()->hasCustomTitle())
            {
                tab_obj.insert("title", tab->getTab()->getCustomTitle());
            }

            // selected
            if (window->getNotebook().getSelectedPage() == tab)
            {
                tab_obj.insert("selected", true);
            }

            // highlighting on new messages
            tab_obj.insert("highlightsEnabled",
                           tab->getTab()->hasHighlightsEnabled());

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
    QString settingsPath = getPaths()->settingsDirectory + SETTINGS_FILENAME;
    QSaveFile file(settingsPath);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);

    QJsonDocument::JsonFormat format =
#ifdef _DEBUG
        QJsonDocument::JsonFormat::Compact
#else
        (QJsonDocument::JsonFormat)0
#endif
        ;

    file.write(document.toJson(format));
    file.commit();
}

void WindowManager::sendAlert()
{
    int flashDuration = 2500;
    if (getSettings()->longAlerts)
    {
        flashDuration = 0;
    }
    QApplication::alert(this->getMainWindow().window(), flashDuration);
}

void WindowManager::queueSave()
{
    using namespace std::chrono_literals;

    this->saveTimer->start(10s);
}

void WindowManager::encodeNodeRecusively(SplitNode *node, QJsonObject &obj)
{
    switch (node->getType())
    {
        case SplitNode::_Split:
        {
            obj.insert("type", "split");
            obj.insert("moderationMode", node->getSplit()->getModerationMode());
            QJsonObject split;
            encodeChannel(node->getSplit()->getIndirectChannel(), split);
            obj.insert("data", split);
            obj.insert("flexh", node->getHorizontalFlex());
            obj.insert("flexv", node->getVerticalFlex());
        }
        break;
        case SplitNode::HorizontalContainer:
        case SplitNode::VerticalContainer:
        {
            obj.insert("type", node->getType() == SplitNode::HorizontalContainer
                                   ? "horizontal"
                                   : "vertical");

            QJsonArray items_arr;
            for (const std::unique_ptr<SplitNode> &n : node->getChildren())
            {
                QJsonObject subObj;
                this->encodeNodeRecusively(n.get(), subObj);
                items_arr.append(subObj);
            }
            obj.insert("items", items_arr);
        }
        break;
    }
}

void WindowManager::encodeChannel(IndirectChannel channel, QJsonObject &obj)
{
    assertInGuiThread();

    switch (channel.getType())
    {
        case Channel::Type::Twitch:
        {
            obj.insert("type", "twitch");
            obj.insert("name", channel.get()->getName());
        }
        break;
        case Channel::Type::TwitchMentions:
        {
            obj.insert("type", "mentions");
        }
        break;
        case Channel::Type::TwitchWatching:
        {
            obj.insert("type", "watching");
        }
        break;
        case Channel::Type::TwitchWhispers:
        {
            obj.insert("type", "whispers");
        }
        break;
    }
}

IndirectChannel WindowManager::decodeChannel(const QJsonObject &obj)
{
    assertInGuiThread();

    auto app = getApp();

    QString type = obj.value("type").toString();
    if (type == "twitch")
    {
        return app->twitch.server->getOrAddChannel(
            obj.value("name").toString());
    }
    else if (type == "mentions")
    {
        return app->twitch.server->mentionsChannel;
    }
    else if (type == "watching")
    {
        return app->twitch.server->watchingChannel;
    }
    else if (type == "whispers")
    {
        return app->twitch.server->whispersChannel;
    }

    return Channel::getEmpty();
}

void WindowManager::closeAll()
{
    assertInGuiThread();

    for (Window *window : windows_)
    {
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

}  // namespace chatterino
