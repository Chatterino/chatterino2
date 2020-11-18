#include "singletons/WindowManager.hpp"

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

#include "Application.hpp"
#include "common/Args.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/MessageElement.hpp"
#include "providers/irc/Irc2.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "qlogging.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/Clamp.hpp"
#include "util/CombinePath.hpp"
#include "widgets/AccountSwitchPopup.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

namespace chatterino {
namespace {

    const QString WINDOW_LAYOUT_FILENAME(QStringLiteral("window-layout.json"));

    boost::optional<bool> &shouldMoveOutOfBoundsWindow()
    {
        static boost::optional<bool> x;
        return x;
    }

}  // namespace

using SplitNode = SplitContainer::Node;
using SplitDirection = SplitContainer::Direction;

void WindowManager::showSettingsDialog(QWidget *parent,
                                       SettingsDialogPreference preference)
{
    QTimer::singleShot(80, [parent, preference] {
        SettingsDialog::showDialog(parent, preference);
    });
}

void WindowManager::showAccountSelectPopup(QPoint point)
{
    //    static QWidget *lastFocusedWidget = nullptr;
    static AccountSwitchPopup *w = new AccountSwitchPopup();

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
    w->move(buttonPos.x() - 30, buttonPos.y());
    w->show();
    w->setFocus();
}

WindowManager::WindowManager()
    : windowLayoutFilePath(
          combinePath(getPaths()->settingsDirectory, WINDOW_LAYOUT_FILENAME))
{
    qCDebug(chatterinoWindowmanager) << "init WindowManager";

    auto settings = getSettings();

    this->wordFlagsListener_.addSetting(settings->showTimestamps);
    this->wordFlagsListener_.addSetting(settings->showBadgesGlobalAuthority);
    this->wordFlagsListener_.addSetting(settings->showBadgesChannelAuthority);
    this->wordFlagsListener_.addSetting(settings->showBadgesSubscription);
    this->wordFlagsListener_.addSetting(settings->showBadgesVanity);
    this->wordFlagsListener_.addSetting(settings->showBadgesChatterino);
    this->wordFlagsListener_.addSetting(settings->showBadgesFfz);
    this->wordFlagsListener_.addSetting(settings->enableEmoteImages);
    this->wordFlagsListener_.addSetting(settings->boldUsernames);
    this->wordFlagsListener_.addSetting(settings->lowercaseDomains);
    this->wordFlagsListener_.setCB([this] {
        this->updateWordTypeMask();
    });

    this->saveTimer = new QTimer;

    this->saveTimer->setSingleShot(true);

    QObject::connect(this->saveTimer, &QTimer::timeout, [] {
        getApp()->windows->save();
    });

    this->miscUpdateTimer_.start(100);

    QObject::connect(&this->miscUpdateTimer_, &QTimer::timeout, [this] {
        this->miscUpdate.invoke();
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
    flags.set(settings->showBadgesFfz ? MEF::BadgeFfz : MEF::None);

    // username
    flags.set(MEF::Username);

    // misc
    flags.set(MEF::AlwaysShow);
    flags.set(MEF::Collapsed);
    flags.set(settings->boldUsernames ? MEF::BoldUsername
                                      : MEF::NonBoldUsername);
    flags.set(settings->lowercaseDomains ? MEF::LowercaseLink
                                         : MEF::OriginalLink);
    flags.set(MEF::ChannelPointReward);

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
    qCDebug(chatterinoWindowmanager) << "getting window at bad index" << index;

    return this->windows_.at(index);
}

QPoint WindowManager::emotePopupPos()
{
    return this->emotePopupPos_;
}

void WindowManager::setEmotePopupPos(QPoint pos)
{
    this->emotePopupPos_ = pos;
}

void WindowManager::initialize(Settings &settings, Paths &paths)
{
    assertInGuiThread();

    getApp()->themes->repaintVisibleChatWidgets_.connect([this] {
        this->repaintVisibleChatWidgets();
    });

    assert(!this->initialized_);

    {
        auto windowLayout = this->loadWindowLayoutFromFile();

        this->emotePopupPos_ = windowLayout.emotePopupPos_;

        this->applyWindowLayout(windowLayout);
    }

    // No main window has been created from loading, create an empty one
    if (mainWindow_ == nullptr)
    {
        mainWindow_ = &this->createWindow(WindowType::Main);
        mainWindow_->getNotebook().addPage(true);
    }

    settings.timestampFormat.connect([this](auto, auto) {
        this->layoutChannelViews();
    });

    settings.emoteScale.connect([this](auto, auto) {
        this->forceLayoutChannelViews();
    });

    settings.timestampFormat.connect([this](auto, auto) {
        this->forceLayoutChannelViews();
    });
    settings.alternateMessages.connect([this](auto, auto) {
        this->forceLayoutChannelViews();
    });
    settings.separateMessages.connect([this](auto, auto) {
        this->forceLayoutChannelViews();
    });
    settings.collpseMessagesMinLines.connect([this](auto, auto) {
        this->forceLayoutChannelViews();
    });
    settings.enableRedeemedHighlight.connect([this](auto, auto) {
        this->forceLayoutChannelViews();
    });

    this->initialized_ = true;
}

void WindowManager::save()
{
    if (getArgs().dontSaveSettings)
    {
        return;
    }
    qCDebug(chatterinoWindowmanager) << "[WindowManager] Saving";
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
        auto rect = window->getBounds();

        window_obj.insert("x", rect.x());
        window_obj.insert("y", rect.y());
        window_obj.insert("width", rect.width());
        window_obj.insert("height", rect.height());

        QJsonObject emote_popup_obj;
        emote_popup_obj.insert("x", this->emotePopupPos_.x());
        emote_popup_obj.insert("y", this->emotePopupPos_.y());
        window_obj.insert("emotePopup", emote_popup_obj);

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

            this->encodeNodeRecursively(tab->getBaseNode(), splits);

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
    QSaveFile file(this->windowLayoutFilePath);
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

void WindowManager::encodeNodeRecursively(SplitNode *node, QJsonObject &obj)
{
    switch (node->getType())
    {
        case SplitNode::_Split: {
            obj.insert("type", "split");
            obj.insert("moderationMode", node->getSplit()->getModerationMode());

            QJsonObject split;
            encodeChannel(node->getSplit()->getIndirectChannel(), split);
            obj.insert("data", split);

            QJsonArray filters;
            encodeFilters(node->getSplit(), filters);
            obj.insert("filters", filters);

            obj.insert("flexh", node->getHorizontalFlex());
            obj.insert("flexv", node->getVerticalFlex());
        }
        break;
        case SplitNode::HorizontalContainer:
        case SplitNode::VerticalContainer: {
            obj.insert("type", node->getType() == SplitNode::HorizontalContainer
                                   ? "horizontal"
                                   : "vertical");

            QJsonArray items_arr;
            for (const std::unique_ptr<SplitNode> &n : node->getChildren())
            {
                QJsonObject subObj;
                this->encodeNodeRecursively(n.get(), subObj);
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
        case Channel::Type::Twitch: {
            obj.insert("type", "twitch");
            obj.insert("name", channel.get()->getName());
        }
        break;
        case Channel::Type::TwitchMentions: {
            obj.insert("type", "mentions");
        }
        break;
        case Channel::Type::TwitchWatching: {
            obj.insert("type", "watching");
        }
        break;
        case Channel::Type::TwitchWhispers: {
            obj.insert("type", "whispers");
        }
        break;
        case Channel::Type::Irc: {
            if (auto ircChannel =
                    dynamic_cast<IrcChannel *>(channel.get().get()))
            {
                obj.insert("type", "irc");
                if (ircChannel->server())
                {
                    obj.insert("server", ircChannel->server()->id());
                }
                obj.insert("channel", ircChannel->getName());
            }
        }
        break;
    }
}

void WindowManager::encodeFilters(Split *split, QJsonArray &arr)
{
    assertInGuiThread();

    auto filters = split->getFilters();
    for (const auto &f : filters)
    {
        arr.append(f.toString(QUuid::WithoutBraces));
    }
}

IndirectChannel WindowManager::decodeChannel(const SplitDescriptor &descriptor)
{
    assertInGuiThread();

    auto app = getApp();

    if (descriptor.type_ == "twitch")
    {
        return app->twitch.server->getOrAddChannel(descriptor.channelName_);
    }
    else if (descriptor.type_ == "mentions")
    {
        return app->twitch.server->mentionsChannel;
    }
    else if (descriptor.type_ == "watching")
    {
        return app->twitch.server->watchingChannel;
    }
    else if (descriptor.type_ == "whispers")
    {
        return app->twitch.server->whispersChannel;
    }
    else if (descriptor.type_ == "irc")
    {
        return Irc::instance().getOrAddChannel(descriptor.server_,
                                               descriptor.channelName_);
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

WindowLayout WindowManager::loadWindowLayoutFromFile() const
{
    return WindowLayout::loadFromFile(this->windowLayoutFilePath);
}

void WindowManager::applyWindowLayout(const WindowLayout &layout)
{
    // Set emote popup position
    this->emotePopupPos_ = layout.emotePopupPos_;

    for (const auto &windowData : layout.windows_)
    {
        auto type = windowData.type_;

        Window &window = this->createWindow(type, false);

        if (type == WindowType::Main)
        {
            assert(this->mainWindow_ == nullptr);

            this->mainWindow_ = &window;
        }

        // get geometry
        {
            // out of bounds windows
            auto screens = qApp->screens();
            bool outOfBounds = std::none_of(
                screens.begin(), screens.end(), [&](QScreen *screen) {
                    return screen->availableGeometry().intersects(
                        windowData.geometry_);
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

            if ((!outOfBounds || !should.value()) &&
                windowData.geometry_.x() != -1 &&
                windowData.geometry_.y() != -1 &&
                windowData.geometry_.width() != -1 &&
                windowData.geometry_.height() != -1)
            {
                // Have to offset x by one because qt moves the window 1px too
                // far to the left:w

                window.setInitialBounds({windowData.geometry_.x(),
                                         windowData.geometry_.y(),
                                         windowData.geometry_.width(),
                                         windowData.geometry_.height()});
            }
        }

        // open tabs
        for (const auto &tab : windowData.tabs_)
        {
            SplitContainer *page = window.getNotebook().addPage(false);

            // set custom title
            if (!tab.customTitle_.isEmpty())
            {
                page->getTab()->setCustomTitle(tab.customTitle_);
            }

            // selected
            if (tab.selected_)
            {
                window.getNotebook().select(page);
            }

            // highlighting on new messages
            page->getTab()->setHighlightsEnabled(tab.highlightsEnabled_);

            if (tab.rootNode_)
            {
                page->applyFromDescriptor(*tab.rootNode_);
            }
        }
        window.show();

        // Set window state
        switch (windowData.state_)
        {
            case WindowDescriptor::State::Minimized: {
                window.setWindowState(Qt::WindowMinimized);
            }
            break;

            case WindowDescriptor::State::Maximized: {
                window.setWindowState(Qt::WindowMaximized);
            }
            break;
        }
    }
}

}  // namespace chatterino
