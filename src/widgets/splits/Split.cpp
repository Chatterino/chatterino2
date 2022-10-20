#include "widgets/splits/Split.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "controllers/notifications/NotificationController.hpp"
#include "messages/MessageThread.hpp"
#include "providers/twitch/EmoteValue.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/Helpers.hpp"
#include "util/NuulsUploader.hpp"
#include "util/StreamLink.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/QualityPopup.hpp"
#include "widgets/dialogs/SelectChannelDialog.hpp"
#include "widgets/dialogs/SelectChannelFiltersDialog.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/DebugPopup.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/helper/SearchPopup.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/splits/SplitHeader.hpp"
#include "widgets/splits/SplitInput.hpp"
#include "widgets/splits/SplitOverlay.hpp"

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
#include <QVBoxLayout>

#include <functional>
#include <random>

namespace chatterino {
namespace {
    void showTutorialVideo(QWidget *parent, const QString &source,
                           const QString &title, const QString &description)
    {
        auto window =
            new BasePopup(BaseWindow::Flags::EnableCustomFrame, parent);
        window->setWindowTitle("Chatterino - " + title);
        window->setAttribute(Qt::WA_DeleteOnClose);
        auto layout = new QVBoxLayout();
        layout->addWidget(new QLabel(description));
        auto label = new QLabel(window);
        layout->addWidget(label);
        auto movie = new QMovie(label);
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
    , view_(new ChannelView(this, this))
    , input_(new SplitInput(this))
    , overlay_(new SplitOverlay(this))
{
    this->setMouseTracking(true);
    this->view_->setPausable(true);
    this->view_->setFocusProxy(this->input_->ui_.textEdit);
    this->setFocusProxy(this->input_->ui_.textEdit);

    this->vbox_->setSpacing(0);
    this->vbox_->setMargin(1);

    this->vbox_->addWidget(this->header_);
    this->vbox_->addWidget(this->view_, 1);
    this->vbox_->addWidget(this->input_);

    this->input_->ui_.textEdit->installEventFilter(parent);

    // update placeholder text on Twitch account change and channel change
    this->bSignals_.emplace_back(
        getApp()->accounts->twitch.currentUserChanged.connect([this] {
            this->updateInputPlaceholder();
        }));
    this->signalHolder_.managedConnect(channelChanged, [this] {
        this->updateInputPlaceholder();
    });
    this->updateInputPlaceholder();

    this->view_->selectionChanged.connect([this]() {
        if (view_->hasSelection())
        {
            this->input_->clearSelection();
        }
    });

    this->view_->openChannelIn.connect([this](
                                           QString twitchChannel,
                                           FromTwitchLinkOpenChannelIn openIn) {
        ChannelPtr channel = getApp()->twitch->getOrAddChannel(twitchChannel);
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
                    << "Unhandled \"FromTwitchLinkOpenChannelIn\" enum value: "
                    << static_cast<int>(openIn);
        }
    });

    this->input_->textChanged.connect([=](const QString &newText) {
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

    this->header_->updateModerationModeIcon();
    this->overlay_->hide();

    this->setSizePolicy(QSizePolicy::MinimumExpanding,
                        QSizePolicy::MinimumExpanding);

    this->signalHolder_.managedConnect(
        modifierStatusChanged, [this](Qt::KeyboardModifiers status) {
            if ((status ==
                 showSplitOverlayModifiers /*|| status == showAddSplitRegions*/) &&
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
    this->input_->ui_.textEdit->imagePasted.connect(
        [this](const QMimeData *source) {
            if (!getSettings()->imageUploaderEnabled)
                return;

            if (getSettings()->askOnImageUpload.getValue())
            {
                QMessageBox msgBox(this->window());
                msgBox.setWindowTitle("Chatterino");
                msgBox.setText("Image upload");
                msgBox.setInformativeText(
                    "You are uploading an image to a 3rd party service not in "
                    "control of the Chatterino team. You may not be able to "
                    "remove the image from the site. Are you okay with this?");
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.addButton(QMessageBox::Yes);
                msgBox.addButton("Yes, don't ask again", QMessageBox::YesRole);

                msgBox.setDefaultButton(QMessageBox::Yes);

                auto picked = msgBox.exec();
                if (picked == QMessageBox::Cancel)
                {
                    return;
                }
                else if (picked == 0)  // don't ask again button
                {
                    getSettings()->askOnImageUpload.setValue(false);
                }
            }
            upload(source, this->getChannel(), *this->input_->ui_.textEdit);
        });

    getSettings()->imageUploaderEnabled.connect(
        [this](const bool &val) {
            this->setAcceptDrops(val);
        },
        this->signalHolder_);
    this->addShortcuts();
    this->signalHolder_.managedConnect(getApp()->hotkeys->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });
}

void Split::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"delete",
         [this](std::vector<QString>) -> QString {
             this->deleteFromContainer();
             return "";
         }},
        {"changeChannel",
         [this](std::vector<QString>) -> QString {
             this->changeChannel();
             return "";
         }},
        {"showSearch",
         [this](std::vector<QString>) -> QString {
             this->showSearch(true);
             return "";
         }},
        {"showGlobalSearch",
         [this](std::vector<QString>) -> QString {
             this->showSearch(false);
             return "";
         }},
        {"reconnect",
         [this](std::vector<QString>) -> QString {
             this->reconnect();
             return "";
         }},
        {"debug",
         [](std::vector<QString>) -> QString {
             auto *popup = new DebugPopup;
             popup->setAttribute(Qt::WA_DeleteOnClose);
             popup->setWindowTitle("Chatterino - Debug popup");
             popup->show();
             return "";
         }},
        {"focus",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
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
         [this](std::vector<QString>) -> QString {
             this->getChannelView().getScrollBar().scrollToBottom(
                 getSettings()->enableSmoothScrollingNewMessages.getValue());
             return "";
         }},
        {"scrollToTop",
         [this](std::vector<QString>) -> QString {
             this->getChannelView().getScrollBar().scrollToTop(
                 getSettings()->enableSmoothScrollingNewMessages.getValue());
             return "";
         }},
        {"scrollPage",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "scrollPage hotkey called without arguments!";
                 return "scrollPage hotkey called without arguments!";
             }
             auto direction = arguments.at(0);

             auto &scrollbar = this->getChannelView().getScrollBar();
             if (direction == "up")
             {
                 scrollbar.offset(-scrollbar.getLargeChange());
             }
             else if (direction == "down")
             {
                 scrollbar.offset(scrollbar.getLargeChange());
             }
             else
             {
                 qCWarning(chatterinoHotkeys) << "Unknown scroll direction";
             }
             return "";
         }},
        {"pickFilters",
         [this](std::vector<QString>) -> QString {
             this->setFiltersDialog();
             return "";
         }},
        {"startWatching",
         [this](std::vector<QString>) -> QString {
             this->startWatching();
             return "";
         }},
        {"openInBrowser",
         [this](std::vector<QString>) -> QString {
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
         [this](std::vector<QString>) -> QString {
             this->openInStreamlink();
             return "";
         }},
        {"openInCustomPlayer",
         [this](std::vector<QString>) -> QString {
             this->openWithCustomScheme();
             return "";
         }},
        {"openModView",
         [this](std::vector<QString>) -> QString {
             this->openModViewInBrowser();
             return "";
         }},
        {"createClip",
         [this](std::vector<QString>) -> QString {
             // Alt+X: create clip LUL
             if (const auto type = this->getChannel()->getType();
                 type != Channel::Type::Twitch &&
                 type != Channel::Type::TwitchWatching)
             {
                 return "Cannot create clip it non-twitch channel.";
             }

             auto *twitchChannel =
                 dynamic_cast<TwitchChannel *>(this->getChannel().get());

             twitchChannel->createClip();
             return "";
         }},
        {"reloadEmotes",
         [this](std::vector<QString> arguments) -> QString {
             auto reloadChannel = true;
             auto reloadSubscriber = true;
             if (arguments.size() != 0)
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
         [this](std::vector<QString> arguments) -> QString {
             if (!this->getChannel()->isTwitchChannel())
             {
                 return "Cannot set moderation mode in non-twitch channel.";
             }
             auto mode = 2;
             // 0 is off
             // 1 is on
             // 2 is toggle
             if (arguments.size() != 0)
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
                 else
                 {
                     mode = 2;
                 }
             }

             if (mode == 0)
             {
                 this->setModerationMode(false);
             }
             else if (mode == 1)
             {
                 this->setModerationMode(true);
             }
             else
             {
                 this->setModerationMode(!this->getModerationMode());
             }
             return "";
         }},
        {"openViewerList",
         [this](std::vector<QString>) -> QString {
             this->showViewerList();
             return "";
         }},
        {"clearMessages",
         [this](std::vector<QString>) -> QString {
             this->clear();
             return "";
         }},
        {"runCommand",
         [this](std::vector<QString> arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "runCommand hotkey called without arguments!";
                 return "runCommand hotkey called without arguments!";
             }
             QString command = getApp()->commands->execCommand(
                 arguments.at(0).replace('\n', ' '), this->getChannel(), false);
             this->getChannel()->sendMessage(command);
             return "";
         }},
        {"setChannelNotification",
         [this](std::vector<QString> arguments) -> QString {
             if (!this->getChannel()->isTwitchChannel())
             {
                 return "Cannot set channel notifications for non-twitch "
                        "channel.";
             }
             auto mode = 2;
             // 0 is off
             // 1 is on
             // 2 is toggle
             if (arguments.size() != 0)
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
                 else
                 {
                     mode = 2;
                 }
             }

             if (mode == 0)
             {
                 getApp()->notifications->removeChannelNotification(
                     this->getChannel()->getName(), Platform::Twitch);
             }
             else if (mode == 1)
             {
                 getApp()->notifications->addChannelNotification(
                     this->getChannel()->getName(), Platform::Twitch);
             }
             else
             {
                 getApp()->notifications->updateChannelNotification(
                     this->getChannel()->getName(), Platform::Twitch);
             }
             return "";
         }},
    };

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
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

    auto user = getApp()->accounts->twitch.getCurrent();
    QString placeholderText;

    if (user->isAnon())
    {
        placeholderText = "Log in to send messages...";
    }
    else
    {
        placeholderText =
            QString("Send message as %1...")
                .arg(getApp()->accounts->twitch.getCurrent()->getUserName());
    }

    this->input_->ui_.textEdit->setPlaceholderText(placeholderText);
}

void Split::joinChannelInNewTab(ChannelPtr channel)
{
    auto &nb = getApp()->windows->getMainWindow().getNotebook();
    SplitContainer *container = nb.addPage(true);

    Split *split = new Split(container);
    split->setChannel(channel);
    container->appendSplit(split);
}

void Split::openChannelInBrowserPlayer(ChannelPtr channel)
{
    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl(
            "https://player.twitch.tv/?parent=twitch.tv&channel=" +
            twitchChannel->getName());
    }
}

void Split::openChannelInStreamlink(QString channelName)
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
            this->header_->updateModerationModeIcon();
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

    this->header_->updateModerationModeIcon();
    this->header_->updateChannelText();
    this->header_->updateRoomModes();

    if (newChannel.getType() == Channel::Type::Twitch)
    {
        this->header_->setViewersButtonVisible(true);
    }
    else
    {
        this->header_->setViewersButtonVisible(false);
    }

    this->channel_.get()->displayNameChanged.connect([this] {
        this->actionRequested.invoke(Action::RefreshTab);
    });

    this->channelChanged.invoke();
    this->actionRequested.invoke(Action::RefreshTab);

    // Queue up save because: Split channel changed
    getApp()->windows->queueSave();
}

void Split::setModerationMode(bool value)
{
    this->moderationMode_ = value;
    this->header_->updateModerationModeIcon();
    this->view_->queueLayout();
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
    if (this->selectChannelDialog_.hasElement())
    {
        this->selectChannelDialog_->raise();

        return;
    }

    auto dialog = new SelectChannelDialog(this);
    if (!empty)
    {
        dialog->setSelectedChannel(this->getIndirectChannel());
    }
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(dialogTitle);
    dialog->show();
    dialog->closed.connect([=] {
        if (dialog->hasSeletedChannel())
        {
            this->setChannel(dialog->getSelectedChannel());
        }

        callback(dialog->hasSeletedChannel());
        this->selectChannelDialog_ = nullptr;
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
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::keyPressEvent(QKeyEvent *event)
{
    this->view_->unsetCursor();
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::keyReleaseEvent(QKeyEvent *event)
{
    this->view_->unsetCursor();
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::resizeEvent(QResizeEvent *event)
{
    // Queue up save because: Split resized
    getApp()->windows->queueSave();

    BaseWidget::resizeEvent(event);

    this->overlay_->setGeometry(this->rect());
}

void Split::enterEvent(QEvent *event)
{
    this->isMouseOver_ = true;

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());

    if (modifierStatus ==
        showSplitOverlayModifiers /*|| modifierStatus == showAddSplitRegions*/)
    {
        this->overlay_->show();
    }

    this->actionRequested.invoke(Action::ResetMouseStatus);
}

void Split::leaveEvent(QEvent *event)
{
    this->isMouseOver_ = false;

    this->overlay_->hide();

    TooltipWidget::instance()->hide();

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
        showViewerList();
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
    auto app = getApp();
    Window &window = app->windows->createWindow(WindowType::Popup);

    Split *split = new Split(static_cast<SplitContainer *>(
        window.getNotebook().getOrAddSelectedPage()));

    split->setChannel(this->getIndirectChannel());
    split->setModerationMode(this->getModerationMode());
    split->setFilters(this->getFilters());

    window.getNotebook().getOrAddSelectedPage()->appendSplit(split);
    window.show();
}

void Split::clear()
{
    this->view_->clearMessages();
}

void Split::openInBrowser()
{
    auto channel = this->getChannel();

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl("https://twitch.tv/" +
                                  twitchChannel->getName());
    }
}

void Split::openWhispersInBrowser()
{
    auto userName = getApp()->accounts->twitch.getCurrent()->getUserName();
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

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
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
    QString scheme = getSettings()->customURIScheme.getValue();
    if (scheme.isEmpty())
    {
        return;
    }

    const auto channel = this->getChannel().get();

    if (const auto twitchChannel = dynamic_cast<TwitchChannel *>(channel))
    {
        QDesktopServices::openUrl(QString("%1https://twitch.tv/%2")
                                      .arg(scheme)
                                      .arg(twitchChannel->getName()));
    }
}

void Split::showViewerList()
{
    auto viewerDock =
        new QDockWidget("Viewer List - " + this->getChannel()->getName(), this);
    viewerDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    viewerDock->setFeatures(QDockWidget::DockWidgetVerticalTitleBar |
                            QDockWidget::DockWidgetClosable |
                            QDockWidget::DockWidgetFloatable);
    viewerDock->resize(
        0.5 * this->width(),
        this->height() - this->header_->height() - this->input_->height());
    viewerDock->move(0, this->header_->height());

    auto multiWidget = new QWidget(viewerDock);
    auto dockVbox = new QVBoxLayout(viewerDock);
    auto searchBar = new QLineEdit(viewerDock);

    auto chattersList = new QListWidget();
    auto resultList = new QListWidget();

    auto formatListItemText = [](QString text) {
        auto item = new QListWidgetItem();
        item->setText(text);
        item->setFont(getApp()->fonts->getFont(FontStyle::ChatMedium, 1.0));
        return item;
    };

    static QStringList labels = {
        "Broadcaster", "Moderators",        "VIPs",   "Staff",
        "Admins",      "Global Moderators", "Viewers"};
    static QStringList jsonLabels = {"broadcaster", "moderators", "vips",
                                     "staff",       "admins",     "global_mods",
                                     "viewers"};
    auto loadingLabel = new QLabel("Loading...");

    searchBar->setPlaceholderText("Search User...");

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

    QObject::connect(searchBar, &QLineEdit::textEdited, this,
                     performListSearch);

    NetworkRequest::twitchRequest("https://tmi.twitch.tv/group/user/" +
                                  this->getChannel()->getName() + "/chatters")
        .caller(this)
        .onSuccess([=](auto result) -> Outcome {
            auto obj = result.parseJson();
            QJsonObject chattersObj = obj.value("chatters").toObject();

            viewerDock->setWindowTitle(
                QString("Viewer List - %1 (%2 chatters)")
                    .arg(this->getChannel()->getName())
                    .arg(localizeNumbers(obj.value("chatter_count").toInt())));

            loadingLabel->hide();
            for (int i = 0; i < jsonLabels.size(); i++)
            {
                auto currentCategory =
                    chattersObj.value(jsonLabels.at(i)).toArray();
                // If current category of chatters is empty, dont show this
                // category.
                if (currentCategory.empty())
                    continue;

                auto label = formatListItemText(QString("%1 (%2)").arg(
                    labels.at(i), localizeNumbers(currentCategory.size())));
                label->setForeground(this->theme->accent);
                chattersList->addItem(label);
                foreach (const QJsonValue &v, currentCategory)
                {
                    chattersList->addItem(formatListItemText(v.toString()));
                }
                chattersList->addItem(new QListWidgetItem());
            }

            performListSearch();
            return Success;
        })
        .execute();

    QObject::connect(viewerDock, &QDockWidget::topLevelChanged, this, [=]() {
        viewerDock->setMinimumWidth(300);
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
         [viewerDock](std::vector<QString>) -> QString {
             viewerDock->close();
             return "";
         }},
        {"accept", nullptr},
        {"reject", nullptr},
        {"scrollPage", nullptr},
        {"openTab", nullptr},
        {"search",
         [searchBar](std::vector<QString>) -> QString {
             searchBar->setFocus();
             searchBar->selectAll();
             return "";
         }},
    };

    getApp()->hotkeys->shortcutsForCategory(HotkeyCategory::PopupWindow,
                                            actions, viewerDock);

    dockVbox->addWidget(searchBar);
    dockVbox->addWidget(loadingLabel);
    dockVbox->addWidget(chattersList);
    dockVbox->addWidget(resultList);
    resultList->hide();

    multiWidget->setStyleSheet(this->theme->splits.input.styleSheet);
    multiWidget->setLayout(dockVbox);
    viewerDock->setWidget(multiWidget);
    viewerDock->setFloating(true);
    viewerDock->show();
    viewerDock->activateWindow();
}

void Split::openSubPage()
{
    ChannelPtr channel = this->getChannel();

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl(twitchChannel->subscriptionUrl());
    }
}

void Split::copyToClipboard()
{
    crossPlatformCopy(this->view_->getSelectedText());
}

void Split::startWatching()
{
#ifdef USEWEBENGINE
    ChannelPtr _channel = this->getChannel();
    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

    if (tc != nullptr)
    {
        StreamView *view = new StreamView(
            _channel,
            "https://player.twitch.tv/?parent=twitch.tv&channel=" + tc->name);
        view->setAttribute(Qt::WA_DeleteOnClose, true);
        view->show();
    }
#endif
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
    auto &notebook = getApp()->windows->getMainWindow().getNotebook();
    for (int i = 0; i < notebook.getPageCount(); ++i)
    {
        auto container = dynamic_cast<SplitContainer *>(notebook.getPageAt(i));
        for (auto split : container->getSplits())
        {
            popup->addChannel(split->getChannelView());
        }
    }

    popup->show();
}

void Split::reloadChannelAndSubscriberEmotes()
{
    auto channel = this->getChannel();
    getApp()->accounts->twitch.getCurrent()->loadEmotes(channel);

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
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
    if (auto container = dynamic_cast<SplitContainer *>(this->parentWidget()))
    {
        SplitContainer::isDraggingSplit = true;
        SplitContainer::draggingSplit = this;

        auto originalLocation = container->releaseSplit(this);
        auto drag = new QDrag(this);
        auto mimeData = new QMimeData;

        mimeData->setData("chatterino/split", "xD");
        drag->setMimeData(mimeData);

        if (drag->exec(Qt::MoveAction) == Qt::IgnoreAction)
        {
            container->insertSplit(this, originalLocation);
        }

        SplitContainer::isDraggingSplit = false;
    }
}

void Split::setInputReply(const std::shared_ptr<MessageThread> &reply)
{
    this->input_->setReply(reply);
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
