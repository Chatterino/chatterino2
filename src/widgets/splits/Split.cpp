#include "widgets/splits/Split.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/ImageUploader.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/CustomPlayer.hpp"
#include "util/StreamLink.hpp"
#include "widgets/ChatterListWidget.hpp"
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
#include <QDesktopServices>
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
    this->signalHolder_.managedConnect(this->channelChanged, [this] {
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
                case FromTwitchLinkOpenChannelIn::CustomPlayer:
                    this->openChannelInCustomPlayer(twitchChannel);
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
        [this](const bool &showEmptyInput) {
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
             const auto &direction = arguments.at(0);
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
             const auto &direction = arguments.at(0);

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

             twitchChannel->createClip({}, {});
             return "";
         }},
        {"reloadEmotes",
         [this](const std::vector<QString> &arguments) -> QString {
             auto reloadChannel = true;
             auto reloadSubscriber = true;
             if (!arguments.empty())
             {
                 const auto &arg = arguments.at(0);
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
                 const auto &arg = arguments.at(0);
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
             this->openChatterList();
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
                 const auto &arg = arguments.at(0);
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
                 const auto &arg = arguments.at(0);
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

void Split::joinChannelInNewTab(const ChannelPtr &channel)
{
    auto &nb = getApp()->getWindows()->getMainWindow().getNotebook();
    SplitContainer *container = nb.addPage(true);

    auto *split = new Split(container);
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

void Split::openChannelInCustomPlayer(const QString channelName)
{
    openInCustomPlayer(channelName);
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

    QObject::connect(
        this->view_, &ChannelView::messageAddedToChannel, this,
        [this](MessagePtr &message) {
            if (!getSettings()->pulseTextInputOnSelfMessage)
            {
                return;
            }
            auto user = getApp()->getAccounts()->twitch.getCurrent();
            if (!user->isAnon() && message->userID == user->getUserId())
            {
                // A message from yourself was just received in this split
                this->input_->triggerSelfMessageReceived();
            }
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
    else
    {
        dialog->setSelectedChannel({});
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

void Split::enterEvent(QEnterEvent * /*event*/)
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
    this->showChangeChannelPopup(
        "Change channel", false, [this](bool didSelectChannel) {
            if (!didSelectChannel)
            {
                return;
            }

            // After changing channel (i.e. pressing OK in the channel switcher), close all open Chatter Lists
            // We could consider updating the chatter list with the new channel
            for (const auto &w : this->findChildren<ChatterListWidget *>())
            {
                w->close();
            }
        });
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

    auto *split = new Split(window.getNotebook().getOrAddSelectedPage());

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
        QDesktopServices::openUrl("https://www.twitch.tv/" +
                                  twitchChannel->getName());
    }
}

void Split::openWhispersInBrowser()
{
    auto userName = getApp()->getAccounts()->twitch.getCurrent()->getUserName();
    QDesktopServices::openUrl("https://www.twitch.tv/popout/moderator/" +
                              userName + "/whispers");
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
        QDesktopServices::openUrl("https://www.twitch.tv/moderator/" +
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
        this->openChannelInCustomPlayer(twitchChannel->getName());
    }
}

void Split::openChatterList()
{
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

    const auto chatterListWidth = static_cast<int>(this->width() * 0.5);
    const auto chatterListHeight =
        this->height() - this->header_->height() - this->input_->height();

    auto *chatterDock = new ChatterListWidget(twitchChannel, this);

    QObject::connect(chatterDock, &ChatterListWidget::userClicked,
                     [this](const QString &userLogin) {
                         this->view_->showUserInfoPopup(userLogin);
                     });

    chatterDock->resize(chatterListWidth, chatterListHeight);
    widgets::showAndMoveWindowTo(
        chatterDock, this->mapToGlobal(QPoint{0, this->header_->height()}),
        widgets::BoundsChecking::CursorPosition);
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

QList<QUuid> Split::getFilters() const
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
    auto dragRes = drag->exec(Qt::MoveAction);
    if (dragRes != Qt::MoveAction || drag->target() == nullptr)
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
