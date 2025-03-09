#include "widgets/splits/Split.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "messages/MessageThread.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/ImageUploader.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/CustomPlayer.hpp"
#include "util/Helpers.hpp"
#include "util/StreamLink.hpp"
#include "widgets/dialogs/QualityPopup.hpp"
#include "widgets/dialogs/SelectChannelDialog.hpp"
#include "widgets/dialogs/SelectChannelFiltersDialog.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/DebugPopup.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/helper/SearchPopup.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/OverlayWindow.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/splits/DraggedSplit.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/splits/SplitHeader.hpp"
#include "widgets/splits/SplitInput.hpp"
#include "widgets/splits/SplitOverlay.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDrag>
#include <QJsonArray>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QMovie>
#include <QPainter>
#include <QSet>
#include <QVBoxLayout>

#include <functional>
#include <random>

namespace {

using namespace chatterino;

QString formatVIPListError(HelixListVIPsError error, const QString &message)
{
    using Error = HelixListVIPsError;

    QString errorMessage = QString("Failed to list VIPs - ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited by Twitch. Try "
                            "again in a few seconds.";
        }
        break;

        case Error::UserMissingScope: {
            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            // TODO(pajlada): Phrase MISSING_PERMISSION
            errorMessage += "You don't have permission to "
                            "perform that action.";
        }
        break;

        case Error::UserNotBroadcaster: {
            errorMessage +=
                "Due to Twitch restrictions, "
                "this command can only be used by the broadcaster. "
                "To see the list of VIPs you must use the Twitch website.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

QString formatModsError(HelixGetModeratorsError error, QString message)
{
    using Error = HelixGetModeratorsError;

    QString errorMessage = QString("Failed to get moderators: ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage +=
                "Due to Twitch restrictions, "
                "this command can only be used by the broadcaster. "
                "To see the list of mods you must use the Twitch website.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

QString formatChattersError(HelixGetChattersError error, QString message)
{
    using Error = HelixGetChattersError;

    QString errorMessage = QString("Failed to get chatters: ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage +=
                "Due to Twitch restrictions, "
                "this command can only be used by moderators. "
                "To see the list of chatters you must use the Twitch website.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

}  // namespace

namespace chatterino {
namespace {
    void showTutorialVideo(QWidget *parent, const QString &source,
                           const QString &title, const QString &description)
    {
        auto *window = new BasePopup(
            {
                BaseWindow::EnableCustomFrame,
                BaseWindow::BoundsCheckOnShow,
            },
            parent);
        window->setWindowTitle("Chatterino - " + title);
        window->setAttribute(Qt::WA_DeleteOnClose);
        auto *layout = new QVBoxLayout();
        layout->addWidget(new QLabel(description));
        auto *label = new QLabel(window);
        layout->addWidget(label);
        auto *movie = new QMovie(label);
        movie->setFileName(source);
        label->setMovie(movie);
        movie->start();
        window->getLayoutContainer()->setLayout(layout);
        window->show();
    }
}  // namespace

pajlada::Signals::Signal<Qt::KeyboardModifiers> Split::modifierStatusChanged;
Qt::KeyboardModifiers Split::modifierStatus = Qt::NoModifier;

Split::Split(QWidget *parent)
    : BaseWidget(parent)
    , channel_(Channel::getEmpty())
    , vbox_(new QVBoxLayout(this))
    , header_(new SplitHeader(this))
    , view_(new ChannelView(this, this, ChannelView::Context::None,
                            getSettings()->scrollbackSplitLimit))
    , input_(new SplitInput(this))
    , overlay_(new SplitOverlay(this))
{
    this->setMouseTracking(true);
    this->view_->setPausable(true);
    this->view_->setFocusProxy(this->input_->ui_.textEdit);
    this->setFocusProxy(this->input_->ui_.textEdit);

    this->vbox_->setSpacing(0);
    this->vbox_->setContentsMargins(1, 1, 1, 1);

    this->vbox_->addWidget(this->header_);
    this->vbox_->addWidget(this->view_, 1);
    this->vbox_->addWidget(this->input_);

    this->input_->ui_.textEdit->installEventFilter(parent);

    // update placeholder text on Twitch account change and channel change
    this->bSignals_.emplace_back(
        getApp()->getAccounts()->twitch.currentUserChanged.connect([this] {
            this->updateInputPlaceholder();
        }));
    this->signalHolder_.managedConnect(channelChanged, [this] {
        this->updateInputPlaceholder();
    });
    this->updateInputPlaceholder();

    // clear SplitInput selection when selecting in ChannelView
    // this connection can be ignored since the ChannelView is owned by this Split
    std::ignore = this->view_->selectionChanged.connect([this]() {
        if (this->input_->hasSelection())
        {
            this->input_->clearSelection();
        }
    });

    // clear ChannelView selection when selecting in SplitInput
    // this connection can be ignored since the SplitInput is owned by this Split
    std::ignore = this->input_->selectionChanged.connect([this]() {
        if (this->view_->hasSelection())
        {
            this->view_->clearSelection();
        }
    });

    // this connection can be ignored since the ChannelView is owned by this Split
    std::ignore = this->view_->openChannelIn.connect(
        [this](QString twitchChannel, FromTwitchLinkOpenChannelIn openIn) {
            ChannelPtr channel =
                getApp()->getTwitch()->getOrAddChannel(twitchChannel);
            switch (openIn)
            {
                case FromTwitchLinkOpenChannelIn::Split:
                    this->openSplitRequested.invoke(channel);
                    break;
                case FromTwitchLinkOpenChannelIn::Tab:
                    this->joinChannelInNewTab(channel);
                    break;
                case FromTwitchLinkOpenChannelIn::BrowserPlayer:
                    this->openChannelInBrowserPlayer(channel);
                    break;
                case FromTwitchLinkOpenChannelIn::Streamlink:
                    this->openChannelInStreamlink(twitchChannel);
                    break;
                default:
                    qCWarning(chatterinoWidget)
                        << "Unhandled \"FromTwitchLinkOpenChannelIn\" enum "
                           "value: "
                        << static_cast<int>(openIn);
            }
        });

    // this connection can be ignored since the SplitInput is owned by this Split
    std::ignore =
        this->input_->textChanged.connect([this](const QString &newText) {
            if (getSettings()->showEmptyInput)
            {
                // We always show the input regardless of the text, so we can early out here
                return;
            }

            if (newText.isEmpty())
            {
                this->input_->hide();
            }
            else if (this->input_->isHidden())
            {
                // Text updated and the input was previously hidden, show it
                this->input_->show();
            }
        });

    getSettings()->showEmptyInput.connect(
        [this](const bool &showEmptyInput, auto) {
            if (showEmptyInput)
            {
                this->input_->show();
            }
            else
            {
                if (this->input_->getInputText().isEmpty())
                {
                    this->input_->hide();
                }
            }
        },
        this->signalHolder_);

    this->header_->updateIcons();
    this->overlay_->hide();

    this->setSizePolicy(QSizePolicy::MinimumExpanding,
                        QSizePolicy::MinimumExpanding);

    // update moderation button when items changed
    this->signalHolder_.managedConnect(
        getSettings()->moderationActions.delayedItemsChanged, [this] {
            this->refreshModerationMode();
        });

    this->signalHolder_.managedConnect(modifierStatusChanged, [this](
                                                                  Qt::KeyboardModifiers
                                                                      status) {
        if ((status ==
             SHOW_SPLIT_OVERLAY_MODIFIERS /*|| status == showAddSplitRegions*/) &&
            this->isMouseOver_)
        {
            this->overlay_->show();
        }
        else
        {
            this->overlay_->hide();
        }

        if (getSettings()->pauseChatModifier.getEnum() != Qt::NoModifier &&
            status == getSettings()->pauseChatModifier.getEnum())
        {
            this->view_->pause(PauseReason::KeyboardModifier);
        }
        else
        {
            this->view_->unpause(PauseReason::KeyboardModifier);
        }
    });

    this->signalHolder_.managedConnect(this->input_->ui_.textEdit->focused,
                                       [this] {
                                           // Forward textEdit's focused event
                                           this->focused.invoke();
                                       });
    this->signalHolder_.managedConnect(this->input_->ui_.textEdit->focusLost,
                                       [this] {
                                           // Forward textEdit's focusLost event
                                           this->focusLost.invoke();
                                       });

    // this connection can be ignored since the SplitInput is owned by this Split
    std::ignore = this->input_->ui_.textEdit->imagePasted.connect(
        [this](const QMimeData *original) {
            if (!getSettings()->imageUploaderEnabled)
            {
                return;
            }

            auto channel = this->getChannel();
            auto *imageUploader = getApp()->getImageUploader();

            auto [images, imageProcessError] =
                imageUploader->getImages(original);
            if (images.empty())
            {
                channel->addSystemMessage(
                    QString(
                        "An error occurred trying to process your image: %1")
                        .arg(imageProcessError));
                return;
            }

            if (getSettings()->askOnImageUpload.getValue())
            {
                QMessageBox msgBox(this->window());
                msgBox.setWindowTitle("Chatterino");
                msgBox.setText("Image upload");
                msgBox.setInformativeText(
                    "You are uploading an image to a 3rd party service not in "
                    "control of the Chatterino team. You may not be able to "
                    "remove the image from the site. Are you okay with this?");
                auto *cancel = msgBox.addButton(QMessageBox::Cancel);
                auto *yes = msgBox.addButton(QMessageBox::Yes);
                auto *yesDontAskAgain = msgBox.addButton("Yes, don't ask again",
                                                         QMessageBox::YesRole);

                msgBox.setDefaultButton(QMessageBox::Yes);

                msgBox.exec();

                auto *clickedButton = msgBox.clickedButton();
                if (clickedButton == yesDontAskAgain)
                {
                    getSettings()->askOnImageUpload.setValue(false);
                }
                else if (clickedButton == yes)
                {
                    // Continue with image upload
                }
                else if (clickedButton == cancel)
                {
                    // Not continuing with image upload
                    return;
                }
                else
                {
                    // An unknown "button" was pressed - handle it as if cancel was pressed
                    // cancel is already handled as the "escape" option, so this should never happen
                    qCWarning(chatterinoImageuploader)
                        << "Unhandled button pressed:" << clickedButton;
                    return;
                }
            }

            QPointer<ResizingTextEdit> edit = this->input_->ui_.textEdit;
            imageUploader->upload(std::move(images), channel, edit);
        });

    getSettings()->imageUploaderEnabled.connect(
        [this](const bool &val) {
            this->setAcceptDrops(val);
        },
        this->signalHolder_);
    this->addShortcuts();
    this->signalHolder_.managedConnect(getApp()->getHotkeys()->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });
}

void Split::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"delete",
         [this](const std::vector<QString> &) -> QString {
             this->deleteFromContainer();
             return "";
         }},
        {"changeChannel",
         [this](const std::vector<QString> &) -> QString {
             this->changeChannel();
             return "";
         }},
        {"showSearch",
         [this](const std::vector<QString> &) -> QString {
             this->showSearch(true);
             return "";
         }},
        {"showGlobalSearch",
         [this](const std::vector<QString> &) -> QString {
             this->showSearch(false);
             return "";
         }},
        {"reconnect",
         [this](const std::vector<QString> &) -> QString {
             this->reconnect();
             return "";
         }},
        {"debug",
         [](const std::vector<QString> &) -> QString {
             auto *popup = new DebugPopup;
             popup->setAttribute(Qt::WA_DeleteOnClose);
             popup->setWindowTitle("Chatterino - Debug popup");
             popup->show();
             return "";
         }},
        {"focus",
         [this](const std::vector<QString> &arguments) -> QString {
             if (arguments.empty())
             {
                 return "focus action requires only one argument: the "
                        "focus direction Use \"up\", \"above\", \"down\", "
                        "\"below\", \"left\" or \"right\".";
             }
             auto direction = arguments.at(0);
             if (direction == "up" || direction == "above")
             {
                 this->actionRequested.invoke(Action::SelectSplitAbove);
             }
             else if (direction == "down" || direction == "below")
             {
                 this->actionRequested.invoke(Action::SelectSplitBelow);
             }
             else if (direction == "left")
             {
                 this->actionRequested.invoke(Action::SelectSplitLeft);
             }
             else if (direction == "right")
             {
                 this->actionRequested.invoke(Action::SelectSplitRight);
             }
             else
             {
                 return "focus in unknown direction. Use \"up\", "
                        "\"above\", \"down\", \"below\", \"left\" or "
                        "\"right\".";
             }
             return "";
         }},
        {"scrollToBottom",
         [this](const std::vector<QString> &) -> QString {
             this->getChannelView().getScrollBar().scrollToBottom(
                 getSettings()->enableSmoothScrollingNewMessages.getValue());
             return "";
         }},
        {"scrollToTop",
         [this](const std::vector<QString> &) -> QString {
             this->getChannelView().getScrollBar().scrollToTop(
                 getSettings()->enableSmoothScrollingNewMessages.getValue());
             return "";
         }},
        {"scrollPage",
         [this](const std::vector<QString> &arguments) -> QString {
             if (arguments.empty())
             {
                 qCWarning(chatterinoHotkeys)
                     << "scrollPage hotkey called without arguments!";
                 return "scrollPage hotkey called without arguments!";
             }
             auto direction = arguments.at(0);

             auto &scrollbar = this->getChannelView().getScrollBar();
             if (direction == "up")
             {
                 scrollbar.offset(-scrollbar.getPageSize());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getPageSize());
             }
             else
             {
                 qCWarning(chatterinoHotkeys) << "Unknown scroll direction";
             }
             return "";
         }},
        {"pickFilters",
         [this](const std::vector<QString> &) -> QString {
             this->setFiltersDialog();
             return "";
         }},
        {"openInBrowser",
         [this](const std::vector<QString> &) -> QString {
             if (this->getChannel()->getType() == Channel::Type::TwitchWhispers)
             {
                 this->openWhispersInBrowser();
             }
             else
             {
                 this->openInBrowser();
             }

             return "";
         }},
        {"openInStreamlink",
         [this](const std::vector<QString> &) -> QString {
             this->openInStreamlink();
             return "";
         }},
        {"openInCustomPlayer",
         [this](const std::vector<QString> &) -> QString {
             this->openWithCustomScheme();
             return "";
         }},
        {"openPlayerInBrowser",
         [this](const std::vector<QString> &) -> QString {
             this->openBrowserPlayer();
             return "";
         }},
        {"openModView",
         [this](const std::vector<QString> &) -> QString {
             this->openModViewInBrowser();
             return "";
         }},
        {"createClip",
         [this](const std::vector<QString> &) -> QString {
             // Alt+X: create clip LUL
             if (const auto type = this->getChannel()->getType();
                 type != Channel::Type::Twitch &&
                 type != Channel::Type::TwitchWatching)
             {
                 return "Cannot create clips in a non-Twitch channel.";
             }

             auto *twitchChannel =
                 dynamic_cast<TwitchChannel *>(this->getChannel().get());

             twitchChannel->createClip();
             return "";
         }},
        {"reloadEmotes",
         [this](const std::vector<QString> &arguments) -> QString {
             auto reloadChannel = true;
             auto reloadSubscriber = true;
             if (!arguments.empty())
             {
                 auto arg = arguments.at(0);
                 if (arg == "channel")
                 {
                     reloadSubscriber = false;
                 }
                 else if (arg == "subscriber")
                 {
                     reloadChannel = false;
                 }
             }

             if (reloadChannel)
             {
                 this->header_->reloadChannelEmotes();
             }
             if (reloadSubscriber)
             {
                 this->header_->reloadSubscriberEmotes();
             }
             return "";
         }},
        {"setModerationMode",
         [this](const std::vector<QString> &arguments) -> QString {
             if (!this->getChannel()->isTwitchChannel())
             {
                 return "Cannot set moderation mode in a non-Twitch "
                        "channel.";
             }
             auto mode = 2;
             // 0 is off
             // 1 is on
             // 2 is toggle
             if (!arguments.empty())
             {
                 auto arg = arguments.at(0);
                 if (arg == "off")
                 {
                     mode = 0;
                 }
                 else if (arg == "on")
                 {
                     mode = 1;
                 }
             }

             switch (mode)
             {
                 case 0:
                     this->setModerationMode(false);
                     break;
                 case 1:
                     this->setModerationMode(true);
                     break;
                 default:
                     this->setModerationMode(!this->getModerationMode());
             }

             return "";
         }},
        {"openViewerList",
         [this](const std::vector<QString> &) -> QString {
             this->showChatterList();
             return "";
         }},
        {"clearMessages",
         [this](const std::vector<QString> &) -> QString {
             this->clear();
             return "";
         }},
        {"runCommand",
         [this](const std::vector<QString> &arguments) -> QString {
             if (arguments.empty())
             {
                 qCWarning(chatterinoHotkeys)
                     << "runCommand hotkey called without arguments!";
                 return "runCommand hotkey called without arguments!";
             }
             QString requestedText = QString(arguments[0]).replace('\n', ' ');

             QString inputText = this->getInput().getInputText();
             QString message = getApp()->getCommands()->execCustomCommand(
                 requestedText.split(' '), Command{"(hotkey)", requestedText},
                 true, this->getChannel(), nullptr,
                 {
                     {"input.text", inputText},
                 });

             message = getApp()->getCommands()->execCommand(
                 message, this->getChannel(), false);
             this->getChannel()->sendMessage(message);
             return "";
         }},
        {"setChannelNotification",
         [this](const std::vector<QString> &arguments) -> QString {
             if (!this->getChannel()->isTwitchChannel())
             {
                 return "Cannot set channel notifications for a non-Twitch "
                        "channel.";
             }
             auto mode = 2;
             // 0 is off
             // 1 is on
             // 2 is toggle
             if (!arguments.empty())
             {
                 auto arg = arguments.at(0);
                 if (arg == "off")
                 {
                     mode = 0;
                 }
                 else if (arg == "on")
                 {
                     mode = 1;
                 }
             }

             auto *notifications = getApp()->getNotifications();
             const QString channelName = this->getChannel()->getName();
             switch (mode)
             {
                 case 0:
                     notifications->removeChannelNotification(channelName,
                                                              Platform::Twitch);
                     break;
                 case 1:
                     notifications->addChannelNotification(channelName,
                                                           Platform::Twitch);
                     break;
                 default:
                     notifications->updateChannelNotification(channelName,
                                                              Platform::Twitch);
             }
             return "";
         }},
        {"popupOverlay",
         [this](const auto &) -> QString {
             this->showOverlayWindow();
             return {};
         }},
        {"toggleOverlayInertia",
         [this](const auto &args) -> QString {
             if (args.empty())
             {
                 return "No arguments provided to toggleOverlayInertia "
                        "(expected one)";
             }
             const auto &arg = args.front();

             if (arg == "this")
             {
                 if (this->overlayWindow_)
                 {
                     this->overlayWindow_->toggleInertia();
                 }
                 return {};
             }
             if (arg == "thisOrAll")
             {
                 if (this->overlayWindow_)
                 {
                     this->overlayWindow_->toggleInertia();
                 }
                 else
                 {
                     getApp()->getWindows()->toggleAllOverlayInertia();
                 }
                 return {};
             }
             if (arg == "all")
             {
                 getApp()->getWindows()->toggleAllOverlayInertia();
                 return {};
             }
             return {};
         }},
        {"setHighlightSounds",
         [this](const std::vector<QString> &arguments) -> QString {
             if (!this->getChannel()->isTwitchChannel())
             {
                 return "Cannot set highlight sounds in a non-Twitch "
                        "channel.";
             }

             auto mode = 2;
             // 0 is off
             // 1 is on
             // 2 is toggle
             if (!arguments.empty())
             {
                 auto arg = arguments.at(0);
                 if (arg == "off")
                 {
                     mode = 0;
                 }
                 else if (arg == "on")
                 {
                     mode = 1;
                 }
             }

             const QString channel = this->getChannel()->getName();

             switch (mode)
             {
                 case 0:
                     getSettings()->mute(channel);
                     break;
                 case 1:
                     getSettings()->unmute(channel);
                     break;
                 default:
                     getSettings()->toggleMutedChannel(channel);
             }
             return "";
         }},
        {"openSubscriptionPage",
         [this](const auto &) -> QString {
             if (!this->getChannel()->isTwitchChannel())
             {
                 return "Cannot subscribe to a non-Twitch "
                        "channel.";
             }

             this->openSubPage();
             return "";
         }},
    };

    this->shortcuts_ = getApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::Split, actions, this);
}

Split::~Split()
{
    this->usermodeChangedConnection_.disconnect();
    this->roomModeChangedConnection_.disconnect();
    this->channelIDChangedConnection_.disconnect();
    this->indirectChannelChangedConnection_.disconnect();
}

ChannelView &Split::getChannelView()
{
    return *this->view_;
}

SplitInput &Split::getInput()
{
    return *this->input_;
}

void Split::updateInputPlaceholder()
{
    if (!this->getChannel()->isTwitchChannel())
    {
        return;
    }

    auto user = getApp()->getAccounts()->twitch.getCurrent();
    QString placeholderText;

    if (user->isAnon())
    {
        placeholderText = "Log in to send messages...";
    }
    else
    {
        placeholderText = QString("Send message as %1...")
                              .arg(getApp()
                                       ->getAccounts()
                                       ->twitch.getCurrent()
                                       ->getUserName());
    }

    this->input_->ui_.textEdit->setPlaceholderText(placeholderText);
}

void Split::joinChannelInNewTab(ChannelPtr channel)
{
    auto &nb = getApp()->getWindows()->getMainWindow().getNotebook();
    SplitContainer *container = nb.addPage(true);

    Split *split = new Split(container);
    split->setChannel(channel);
    container->insertSplit(split);
}

void Split::refreshModerationMode()
{
    this->header_->updateIcons();
    this->view_->queueLayout();
}

void Split::openChannelInBrowserPlayer(ChannelPtr channel)
{
    if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl(
            QUrl(TWITCH_PLAYER_URL.arg(twitchChannel->getName())));
    }
}

void Split::openChannelInStreamlink(const QString channelName)
{
    try
    {
        openStreamlinkForChannel(channelName);
    }
    catch (const Exception &ex)
    {
        qCWarning(chatterinoWidget)
            << "Error in doOpenStreamlink:" << ex.what();
    }
}

IndirectChannel Split::getIndirectChannel()
{
    return this->channel_;
}

ChannelPtr Split::getChannel() const
{
    return this->channel_.get();
}

void Split::setChannel(IndirectChannel newChannel)
{
    this->channel_ = newChannel;

    this->view_->setChannel(newChannel.get());

    this->usermodeChangedConnection_.disconnect();
    this->roomModeChangedConnection_.disconnect();
    this->indirectChannelChangedConnection_.disconnect();

    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(newChannel.get().get());

    if (tc != nullptr)
    {
        this->usermodeChangedConnection_ = tc->userStateChanged.connect([this] {
            this->header_->updateIcons();
            this->header_->updateRoomModes();
        });

        this->roomModeChangedConnection_ = tc->roomModesChanged.connect([this] {
            this->header_->updateRoomModes();
        });
    }

    this->indirectChannelChangedConnection_ =
        newChannel.getChannelChanged().connect([this] {
            QTimer::singleShot(0, [this] {
                this->setChannel(this->channel_);
            });
        });

    this->header_->updateIcons();
    this->header_->updateChannelText();
    this->header_->updateRoomModes();

    this->channelSignalHolder_.managedConnect(
        this->channel_.get()->displayNameChanged, [this] {
            this->actionRequested.invoke(Action::RefreshTab);
        });

    this->channelChanged.invoke();
    this->actionRequested.invoke(Action::RefreshTab);

    // Queue up save because: Split channel changed
    getApp()->getWindows()->queueSave();
}

void Split::setModerationMode(bool value)
{
    this->moderationMode_ = value;
    this->refreshModerationMode();
}

bool Split::getModerationMode() const
{
    return this->moderationMode_;
}

void Split::insertTextToInput(const QString &text)
{
    this->input_->insertText(text);
}

void Split::showChangeChannelPopup(const char *dialogTitle, bool empty,
                                   std::function<void(bool)> callback)
{
    if (!this->selectChannelDialog_.isNull())
    {
        this->selectChannelDialog_->raise();

        return;
    }

    auto *dialog = new SelectChannelDialog(this);
    if (!empty)
    {
        dialog->setSelectedChannel(this->getIndirectChannel());
    }
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(dialogTitle);
    dialog->show();
    // We can safely ignore this signal connection since the dialog will be closed before
    // this Split is closed
    std::ignore = dialog->closed.connect([=, this] {
        if (dialog->hasSeletedChannel())
        {
            this->setChannel(dialog->getSelectedChannel());
        }

        callback(dialog->hasSeletedChannel());
    });
    this->selectChannelDialog_ = dialog;
}

void Split::updateGifEmotes()
{
    this->view_->queueUpdate();
}

void Split::updateLastReadMessage()
{
    this->view_->updateLastReadMessage();
}

void Split::paintEvent(QPaintEvent *)
{
    // color the background of the chat
    QPainter painter(this);

    painter.fillRect(this->rect(), this->theme->splits.background);
}

void Split::mouseMoveEvent(QMouseEvent *event)
{
    (void)event;

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::keyPressEvent(QKeyEvent *event)
{
    (void)event;

    this->view_->unsetCursor();
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::keyReleaseEvent(QKeyEvent *event)
{
    (void)event;

    this->view_->unsetCursor();
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::resizeEvent(QResizeEvent *event)
{
    // Queue up save because: Split resized
    getApp()->getWindows()->queueSave();

    BaseWidget::resizeEvent(event);

    this->overlay_->setGeometry(this->rect());
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void Split::enterEvent(QEnterEvent * /*event*/)
#else
void Split::enterEvent(QEvent * /*event*/)
#endif
{
    this->isMouseOver_ = true;

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());

    if (modifierStatus ==
        SHOW_SPLIT_OVERLAY_MODIFIERS /*|| modifierStatus == showAddSplitRegions*/)
    {
        this->overlay_->show();
    }

    this->actionRequested.invoke(Action::ResetMouseStatus);
}

void Split::leaveEvent(QEvent *event)
{
    (void)event;

    this->isMouseOver_ = false;

    this->overlay_->hide();

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::handleModifiers(Qt::KeyboardModifiers modifiers)
{
    if (modifierStatus != modifiers)
    {
        modifierStatus = modifiers;
        modifierStatusChanged.invoke(modifiers);
    }
}

void Split::setIsTopRightSplit(bool value)
{
    this->isTopRightSplit_ = value;
    this->header_->setAddButtonVisible(value);
}

/// Slots
void Split::addSibling()
{
    this->actionRequested.invoke(Action::AppendNewSplit);
}

void Split::deleteFromContainer()
{
    this->actionRequested.invoke(Action::Delete);
}

void Split::changeChannel()
{
    this->showChangeChannelPopup("Change channel", false, [](bool) {});

    auto popup = this->findChildren<QDockWidget *>();
    if (popup.size() && popup.at(0)->isVisible() && !popup.at(0)->isFloating())
    {
        popup.at(0)->hide();
        showChatterList();
    }
}

void Split::explainMoving()
{
    showTutorialVideo(this, ":/examples/moving.gif", "Moving",
                      "Hold <Ctrl+Alt> to move splits.\n\nExample:");
}

void Split::explainSplitting()
{
    showTutorialVideo(this, ":/examples/splitting.gif", "Splitting",
                      "Hold <Ctrl+Alt> to add new splits.\n\nExample:");
}

void Split::popup()
{
    auto *app = getApp();
    Window &window = app->getWindows()->createWindow(WindowType::Popup);

    Split *split = new Split(static_cast<SplitContainer *>(
        window.getNotebook().getOrAddSelectedPage()));

    split->setChannel(this->getIndirectChannel());
    split->setModerationMode(this->getModerationMode());
    split->setFilters(this->getFilters());

    window.getNotebook().getOrAddSelectedPage()->insertSplit(split);
    window.show();
}

OverlayWindow *Split::overlayWindow()
{
    return this->overlayWindow_.data();
}

void Split::showOverlayWindow()
{
    if (!this->overlayWindow_)
    {
        this->overlayWindow_ =
            new OverlayWindow(this->getIndirectChannel(), this->getFilters());
    }
    this->overlayWindow_->show();
}

void Split::clear()
{
    this->view_->clearMessages();
}

void Split::openInBrowser()
{
    auto channel = this->getChannel();

    if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl("https://twitch.tv/" +
                                  twitchChannel->getName());
    }
}

void Split::openWhispersInBrowser()
{
    auto userName = getApp()->getAccounts()->twitch.getCurrent()->getUserName();
    QDesktopServices::openUrl("https://twitch.tv/popout/moderator/" + userName +
                              "/whispers");
}

void Split::openBrowserPlayer()
{
    this->openChannelInBrowserPlayer(this->getChannel());
}

void Split::openModViewInBrowser()
{
    auto channel = this->getChannel();

    if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl("https://twitch.tv/moderator/" +
                                  twitchChannel->getName());
    }
}

void Split::openInStreamlink()
{
    this->openChannelInStreamlink(this->getChannel()->getName());
}

void Split::openWithCustomScheme()
{
    auto *const channel = this->getChannel().get();

    if (auto *const twitchChannel = dynamic_cast<TwitchChannel *>(channel))
    {
        openInCustomPlayer(twitchChannel->getName());
    }
}

void Split::showChatterList()
{
    auto *chatterDock = new QDockWidget(
        "Chatter List - " + this->getChannel()->getName(), this);
    chatterDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    chatterDock->setFeatures(QDockWidget::DockWidgetVerticalTitleBar |
                             QDockWidget::DockWidgetClosable |
                             QDockWidget::DockWidgetFloatable);
    chatterDock->resize(
        0.5 * this->width(),
        this->height() - this->header_->height() - this->input_->height());
    chatterDock->move(0, this->header_->height());

    auto *multiWidget = new QWidget(chatterDock);
    auto *dockVbox = new QVBoxLayout();
    auto *searchBar = new QLineEdit(chatterDock);

    auto *chattersList = new QListWidget();
    auto *resultList = new QListWidget();

    auto channel = this->getChannel();
    if (!channel)
    {
        qCWarning(chatterinoWidget)
            << "Chatter list opened when no channel was defined";
        return;
    }

    auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get());

    if (twitchChannel == nullptr)
    {
        qCWarning(chatterinoWidget)
            << "Chatter list opened in a non-Twitch channel";
        return;
    }

    auto *loadingLabel = new QLabel("Loading...");
    searchBar->setPlaceholderText("Search User...");

    auto formatListItemText = [](QString text) {
        auto *item = new QListWidgetItem();
        item->setText(text);
        item->setFont(
            getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1.0));
        return item;
    };

    auto addLabel = [this, formatListItemText, chattersList](QString label) {
        auto *formattedLabel = formatListItemText(label);
        formattedLabel->setForeground(this->theme->accent);
        chattersList->addItem(formattedLabel);
    };

    auto addUserList = [=](QStringList users, QString label) {
        if (users.isEmpty())
        {
            return;
        }

        addLabel(QString("%1 (%2)").arg(label, localizeNumbers(users.size())));

        for (const auto &user : users)
        {
            chattersList->addItem(formatListItemText(user));
        }
        chattersList->addItem(new QListWidgetItem());
    };

    auto performListSearch = [=]() {
        auto query = searchBar->text();
        if (query.isEmpty())
        {
            resultList->hide();
            chattersList->show();
            return;
        }

        auto results = chattersList->findItems(query, Qt::MatchContains);
        chattersList->hide();
        resultList->clear();
        for (auto &item : results)
        {
            if (!item->text().contains("("))
            {
                resultList->addItem(formatListItemText(item->text()));
            }
        }
        resultList->show();
    };

    auto loadChatters = [=](auto modList, auto vipList, bool isBroadcaster) {
        getHelix()->getChatters(
            twitchChannel->roomId(),
            getApp()->getAccounts()->twitch.getCurrent()->getUserId(), 50000,
            [=](auto chatters) {
                auto broadcaster = channel->getName().toLower();
                QStringList chatterList;
                QStringList modChatters;
                QStringList vipChatters;

                bool addedBroadcaster = false;
                for (auto chatter : chatters.chatters)
                {
                    chatter = chatter.toLower();

                    if (!addedBroadcaster && chatter == broadcaster)
                    {
                        addedBroadcaster = true;
                        addLabel("Broadcaster");
                        chattersList->addItem(broadcaster);
                        chattersList->addItem(new QListWidgetItem());
                        continue;
                    }

                    if (modList.contains(chatter))
                    {
                        modChatters.append(chatter);
                        continue;
                    }

                    if (vipList.contains(chatter))
                    {
                        vipChatters.append(chatter);
                        continue;
                    }

                    chatterList.append(chatter);
                }

                modChatters.sort();
                vipChatters.sort();
                chatterList.sort();

                if (isBroadcaster)
                {
                    addUserList(modChatters, QString("Moderators"));
                    addUserList(vipChatters, QString("VIPs"));
                }
                else
                {
                    addLabel("Moderators");
                    chattersList->addItem(
                        "Moderators cannot check who is a moderator");
                    chattersList->addItem(new QListWidgetItem());

                    addLabel("VIPs");
                    chattersList->addItem(
                        "Moderators cannot check who is a VIP");
                    chattersList->addItem(new QListWidgetItem());
                }

                addUserList(chatterList, QString("Chatters"));

                loadingLabel->hide();
                performListSearch();
            },
            [chattersList, formatListItemText](auto error, auto message) {
                auto errorMessage = formatChattersError(error, message);
                chattersList->addItem(formatListItemText(errorMessage));
            });
    };

    QObject::connect(searchBar, &QLineEdit::textEdited, this,
                     performListSearch);

    // Only broadcaster can get vips, mods can get chatters
    if (channel->isBroadcaster())
    {
        // Add moderators
        getHelix()->getModerators(
            twitchChannel->roomId(), 1000,
            [=](auto mods) {
                QSet<QString> modList;
                for (const auto &mod : mods)
                {
                    modList.insert(mod.userName.toLower());
                }

                // Add vips
                getHelix()->getChannelVIPs(
                    twitchChannel->roomId(),
                    [=](auto vips) {
                        QSet<QString> vipList;
                        for (const auto &vip : vips)
                        {
                            vipList.insert(vip.userName.toLower());
                        }

                        // Add chatters
                        loadChatters(modList, vipList, true);
                    },
                    [chattersList, formatListItemText](auto error,
                                                       auto message) {
                        auto errorMessage = formatVIPListError(error, message);
                        chattersList->addItem(formatListItemText(errorMessage));
                    });
            },
            [chattersList, formatListItemText](auto error, auto message) {
                auto errorMessage = formatModsError(error, message);
                chattersList->addItem(formatListItemText(errorMessage));
            });
    }
    else if (channel->hasModRights())
    {
        QSet<QString> modList;
        QSet<QString> vipList;
        loadChatters(modList, vipList, false);
    }
    else
    {
        chattersList->addItem(
            formatListItemText("Due to Twitch restrictions, this feature is "
                               "only \navailable for moderators."));
        chattersList->addItem(
            formatListItemText("If you would like to see the Chatter list, you "
                               "must \nuse the Twitch website."));
        loadingLabel->hide();
    }

    QObject::connect(chatterDock, &QDockWidget::topLevelChanged, this, [=]() {
        chatterDock->setMinimumWidth(300);
    });

    auto listDoubleClick = [this](const QModelIndex &index) {
        const auto itemText = index.data().toString();

        // if the list item contains a parentheses it means that
        // it's a category label so don't show a usercard
        if (!itemText.contains("(") && !itemText.isEmpty())
        {
            this->view_->showUserInfoPopup(itemText);
        }
    };

    QObject::connect(chattersList, &QListWidget::doubleClicked, this,
                     listDoubleClick);

    QObject::connect(resultList, &QListWidget::doubleClicked, this,
                     listDoubleClick);

    HotkeyController::HotkeyMap actions{
        {"delete",
         [chatterDock](const std::vector<QString> &) -> QString {
             chatterDock->close();
             return "";
         }},
        {"accept", nullptr},
        {"reject", nullptr},
        {"scrollPage", nullptr},
        {"openTab", nullptr},
        {"search",
         [searchBar](const std::vector<QString> &) -> QString {
             searchBar->setFocus();
             searchBar->selectAll();
             return "";
         }},
    };

    getApp()->getHotkeys()->shortcutsForCategory(HotkeyCategory::PopupWindow,
                                                 actions, chatterDock);

    dockVbox->addWidget(searchBar);
    dockVbox->addWidget(loadingLabel);
    dockVbox->addWidget(chattersList);
    dockVbox->addWidget(resultList);
    resultList->hide();

    multiWidget->setStyleSheet(this->theme->splits.input.styleSheet);
    multiWidget->setLayout(dockVbox);
    chatterDock->setWidget(multiWidget);
    chatterDock->setFloating(true);
    widgets::showAndMoveWindowTo(
        chatterDock, this->mapToGlobal(QPoint{0, this->header_->height()}),
        widgets::BoundsChecking::CursorPosition);
    chatterDock->activateWindow();
}

void Split::openSubPage()
{
    ChannelPtr channel = this->getChannel();

    if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl(twitchChannel->subscriptionUrl());
    }
}

void Split::setFiltersDialog()
{
    SelectChannelFiltersDialog d(this->getFilters(), this);
    d.setWindowTitle("Select filters");

    if (d.exec() == QDialog::Accepted)
    {
        this->setFilters(d.getSelection());
    }
}

void Split::setFilters(const QList<QUuid> ids)
{
    this->view_->setFilters(ids);
    this->header_->updateChannelText();
}

const QList<QUuid> Split::getFilters() const
{
    return this->view_->getFilterIds();
}

void Split::showSearch(bool singleChannel)
{
    auto *popup = new SearchPopup(this, this);
    popup->setAttribute(Qt::WA_DeleteOnClose);

    if (singleChannel)
    {
        popup->addChannel(this->getChannelView());
        popup->show();
        return;
    }

    // Pass every ChannelView for every Split across the app to the search popup
    auto &notebook = getApp()->getWindows()->getMainWindow().getNotebook();
    for (int i = 0; i < notebook.getPageCount(); ++i)
    {
        auto *container = dynamic_cast<SplitContainer *>(notebook.getPageAt(i));
        for (auto *split : container->getSplits())
        {
            if (split->channel_.getType() != Channel::Type::TwitchAutomod)
            {
                popup->addChannel(split->getChannelView());
            }
        }
    }

    popup->show();
}

void Split::reloadChannelAndSubscriberEmotes()
{
    auto channel = this->getChannel();

    if (auto *twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        twitchChannel->refreshTwitchChannelEmotes(true);
        twitchChannel->refreshBTTVChannelEmotes(true);
        twitchChannel->refreshFFZChannelEmotes(true);
        twitchChannel->refreshSevenTVChannelEmotes(true);
    }
}

void Split::reconnect()
{
    this->getChannel()->reconnect();
}

void Split::dragEnterEvent(QDragEnterEvent *event)
{
    if (getSettings()->imageUploaderEnabled &&
        (event->mimeData()->hasImage() || event->mimeData()->hasUrls()))
    {
        event->acceptProposedAction();
    }
    else
    {
        BaseWidget::dragEnterEvent(event);
    }
}

void Split::dropEvent(QDropEvent *event)
{
    if (getSettings()->imageUploaderEnabled &&
        (event->mimeData()->hasImage() || event->mimeData()->hasUrls()))
    {
        this->input_->ui_.textEdit->imagePasted.invoke(event->mimeData());
    }
    else
    {
        BaseWidget::dropEvent(event);
    }
}
template <typename Iter, typename RandomGenerator>
static Iter select_randomly(Iter start, Iter end, RandomGenerator &g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template <typename Iter>
static Iter select_randomly(Iter start, Iter end)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

void Split::drag()
{
    auto *container = dynamic_cast<SplitContainer *>(this->parentWidget());
    if (!container)
    {
        qCWarning(chatterinoWidget) << "Attempted to initiate split drag "
                                       "without a container parent";
        return;
    }

    startDraggingSplit();

    auto originalLocation = container->releaseSplit(this);
    auto *drag = new QDrag(this);
    auto *mimeData = new QMimeData;

    mimeData->setData("chatterino/split", "xD");
    drag->setMimeData(mimeData);

    // drag->exec is a blocking action
    if (drag->exec(Qt::MoveAction) == Qt::IgnoreAction)
    {
        // The split wasn't dropped in a valid spot, return it to its original position
        container->insertSplit(this, {.position = originalLocation});
    }

    stopDraggingSplit();
}

void Split::setInputReply(const MessagePtr &reply)
{
    this->input_->setReply(reply);
}

void Split::unpause()
{
    this->view_->unpause(PauseReason::KeyboardModifier);
    this->view_->unpause(PauseReason::DoubleClick);
    // Mouse intentionally left out, we may still have the mouse over the split
}

}  // namespace chatterino

QDebug operator<<(QDebug dbg, const chatterino::Split &split)
{
    auto channel = split.getChannel();
    if (channel)
    {
        dbg.nospace() << "Split(" << (void *)&split
                      << ", channel:" << channel->getName() << ")";
    }
    else
    {
        dbg.nospace() << "Split(" << (void *)&split << ", no channel)";
    }

    return dbg;
}

QDebug operator<<(QDebug dbg, const chatterino::Split *split)
{
    if (split != nullptr)
    {
        return operator<<(dbg, *split);
    }

    dbg.nospace() << "Split(nullptr)";

    return dbg;
}
