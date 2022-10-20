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

#include <QMessageBox>
#include "Application.hpp"
#include "common/Args.hpp"
#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "messages/MessageElement.hpp"
#include "providers/irc/Irc2.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/Clamp.hpp"
#include "util/CombinePath.hpp"
#include "widgets/AccountSwitchPopup.hpp"
#include "widgets/FramelessEmbedWindow.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

namespace chatterino {
namespace {

    boost::optional<bool> &shouldMoveOutOfBoundsWindow()
    {
        static boost::optional<bool> x;
        return x;
    }

}  // namespace

const QString WindowManager::WINDOW_LAYOUT_FILENAME(
    QStringLiteral("window-layout.json"));

using SplitNode = SplitContainer::Node;
using SplitDirection = SplitContainer::Direction;

void WindowManager::showSettingsDialog(QWidget *parent,
                                       SettingsDialogPreference preference)
{
    if (getArgs().dontSaveSettings)
    {
        QMessageBox::critical(parent, "Chatterino - Editing Settings Forbidden",
                              "Settings cannot be edited when running with\n"
                              "commandline arguments such as '-c'.");
    }
    else
    {
        QTimer::singleShot(80, [parent, preference] {
            SettingsDialog::showDialog(parent, preference);
        });
    }
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
    : windowLayoutFilePath(combinePath(getPaths()->settingsDirectory,
                                       WindowManager::WINDOW_LAYOUT_FILENAME))
{
    qCDebug(chatterinoWindowmanager) << "init WindowManager";

    auto settings = getSettings();

    this->wordFlagsListener_.addSetting(settings->showTimestamps);
    this->wordFlagsListener_.addSetting(settings->showBadgesGlobalAuthority);
    this->wordFlagsListener_.addSetting(settings->showBadgesPredictions);
    this->wordFlagsListener_.addSetting(settings->showBadgesChannelAuthority);
    this->wordFlagsListener_.addSetting(settings->showBadgesSubscription);
    this->wordFlagsListener_.addSetting(settings->showBadgesVanity);
    this->wordFlagsListener_.addSetting(settings->showBadgesChatterino);
    this->wordFlagsListener_.addSetting(settings->showBadgesFfz);
    this->wordFlagsListener_.addSetting(settings->showBadgesSevenTV);
    this->wordFlagsListener_.addSetting(settings->enableEmoteImages);
    this->wordFlagsListener_.addSetting(settings->boldUsernames);
    this->wordFlagsListener_.addSetting(settings->lowercaseDomains);
    this->wordFlagsListener_.addSetting(settings->showReplyButton);
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

WindowManager::~WindowManager() = default;

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
    flags.set(settings->showBadgesPredictions ? MEF::BadgePredictions
                                              : MEF::None);
    flags.set(settings->showBadgesChannelAuthority ? MEF::BadgeChannelAuthority
                                                   : MEF::None);
    flags.set(settings->showBadgesSubscription ? MEF::BadgeSubscription
                                               : MEF::None);
    flags.set(settings->showBadgesVanity ? MEF::BadgeVanity : MEF::None);
    flags.set(settings->showBadgesChatterino ? MEF::BadgeChatterino
                                             : MEF::None);
    flags.set(settings->showBadgesFfz ? MEF::BadgeFfz : MEF::None);
    flags.set(settings->showBadgesSevenTV ? MEF::BadgeSevenTV : MEF::None);

    // username
    flags.set(MEF::Username);

    // replies
    flags.set(MEF::RepliedMessage);
    flags.set(settings->showReplyButton ? MEF::ReplyButton : MEF::None);

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

Window &WindowManager::createWindow(WindowType type, bool show, QWidget *parent)
{
    assertInGuiThread();

    auto *const realParent = [this, type, parent]() -> QWidget * {
        if (parent)
        {
            // If a parent is explicitly specified, we use that immediately.
            return parent;
        }

        if (type == WindowType::Popup)
        {
            // On some window managers, popup windows require a parent to behave correctly. See
            // https://github.com/Chatterino/chatterino2/pull/1843 for additional context.
            return &(this->getMainWindow());
        }

        // If no parent is set and something other than a popup window is being created, we fall
        // back to the default behavior of no parent.
        return nullptr;
    }();

    auto *window = new Window(type, realParent);

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

Window &WindowManager::openInPopup(ChannelPtr channel)
{
    auto &popup = this->createWindow(WindowType::Popup, true);
    auto *split =
        popup.getNotebook().getOrAddSelectedPage()->appendNewSplit(false);
    split->setChannel(channel);

    return popup;
}

void WindowManager::select(Split *split)
{
    this->selectSplit.invoke(split);
}

void WindowManager::select(SplitContainer *container)
{
    this->selectSplitContainer.invoke(container);
}

void WindowManager::scrollToMessage(const MessagePtr &message)
{
    this->scrollToMessageSignal.invoke(message);
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
        WindowLayout windowLayout;

        if (getArgs().customChannelLayout)
        {
            windowLayout = getArgs().customChannelLayout.value();
        }
        else
        {
            windowLayout = this->loadWindowLayoutFromFile();
        }

        this->emotePopupPos_ = windowLayout.emotePopupPos_;

        this->applyWindowLayout(windowLayout);
    }

    if (getArgs().isFramelessEmbed)
    {
        this->framelessEmbedWindow_.reset(new FramelessEmbedWindow);
        this->framelessEmbedWindow_->show();
    }

    // No main window has been created from loading, create an empty one
    if (this->mainWindow_ == nullptr)
    {
        this->mainWindow_ = &this->createWindow(WindowType::Main);
        this->mainWindow_->getNotebook().addPage(true);

        // TODO: don't create main window if it's a frameless embed
        if (getArgs().isFramelessEmbed)
        {
            this->mainWindow_->hide();
        }
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
    QJsonArray windowArr;
    for (Window *window : this->windows_)
    {
        QJsonObject windowObj;

        // window type
        switch (window->getType())
        {
            case WindowType::Main:
                windowObj.insert("type", "main");
                break;

            case WindowType::Popup:
                windowObj.insert("type", "popup");
                break;

            case WindowType::Attached:;
        }

        if (window->isMaximized())
        {
            windowObj.insert("state", "maximized");
        }
        else if (window->isMinimized())
        {
            windowObj.insert("state", "minimized");
        }

        // window geometry
        auto rect = window->getBounds();

        windowObj.insert("x", rect.x());
        windowObj.insert("y", rect.y());
        windowObj.insert("width", rect.width());
        windowObj.insert("height", rect.height());

        QJsonObject emotePopupObj;
        emotePopupObj.insert("x", this->emotePopupPos_.x());
        emotePopupObj.insert("y", this->emotePopupPos_.y());
        windowObj.insert("emotePopup", emotePopupObj);

        // window tabs
        QJsonArray tabsArr;

        for (int tabIndex = 0; tabIndex < window->getNotebook().getPageCount();
             tabIndex++)
        {
            QJsonObject tabObj;
            SplitContainer *tab = dynamic_cast<SplitContainer *>(
                window->getNotebook().getPageAt(tabIndex));
            assert(tab != nullptr);

            bool isSelected = window->getNotebook().getSelectedPage() == tab;
            WindowManager::encodeTab(tab, isSelected, tabObj);
            tabsArr.append(tabObj);
        }

        windowObj.insert("tabs", tabsArr);
        windowArr.append(windowObj);
    }

    QJsonObject obj;
    obj.insert("windows", windowArr);
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

void WindowManager::encodeTab(SplitContainer *tab, bool isSelected,
                              QJsonObject &obj)
{
    // custom tab title
    if (tab->getTab()->hasCustomTitle())
    {
        obj.insert("title", tab->getTab()->getCustomTitle());
    }

    // selected
    if (isSelected)
    {
        obj.insert("selected", true);
    }

    // highlighting on new messages
    obj.insert("highlightsEnabled", tab->getTab()->hasHighlightsEnabled());

    // splits
    QJsonObject splits;

    WindowManager::encodeNodeRecursively(tab->getBaseNode(), splits);

    obj.insert("splits2", splits);
}

void WindowManager::encodeNodeRecursively(SplitNode *node, QJsonObject &obj)
{
    switch (node->getType())
    {
        case SplitNode::_Split: {
            obj.insert("type", "split");
            obj.insert("moderationMode", node->getSplit()->getModerationMode());

            QJsonObject split;
            WindowManager::encodeChannel(node->getSplit()->getIndirectChannel(),
                                         split);
            obj.insert("data", split);

            QJsonArray filters;
            WindowManager::encodeFilters(node->getSplit(), filters);
            obj.insert("filters", filters);
        }
        break;
        case SplitNode::HorizontalContainer:
        case SplitNode::VerticalContainer: {
            obj.insert("type", node->getType() == SplitNode::HorizontalContainer
                                   ? "horizontal"
                                   : "vertical");

            QJsonArray itemsArr;
            for (const std::unique_ptr<SplitNode> &n : node->getChildren())
            {
                QJsonObject subObj;
                WindowManager::encodeNodeRecursively(n.get(), subObj);
                itemsArr.append(subObj);
            }
            obj.insert("items", itemsArr);
        }
        break;
    }

    obj.insert("flexh", node->getHorizontalFlex());
    obj.insert("flexv", node->getVerticalFlex());
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
        case Channel::Type::TwitchLive: {
            obj.insert("type", "live");
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
        return app->twitch->getOrAddChannel(descriptor.channelName_);
    }
    else if (descriptor.type_ == "mentions")
    {
        return app->twitch->mentionsChannel;
    }
    else if (descriptor.type_ == "watching")
    {
        return app->twitch->watchingChannel;
    }
    else if (descriptor.type_ == "whispers")
    {
        return app->twitch->whispersChannel;
    }
    else if (descriptor.type_ == "live")
    {
        return app->twitch->liveChannel;
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
    if (getArgs().dontLoadMainWindow)
    {
        return;
    }

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
            bool outOfBounds =
                !getenv("I3SOCK") &&
                std::none_of(screens.begin(), screens.end(),
                             [&](QScreen *screen) {
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
