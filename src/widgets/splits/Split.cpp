#include "widgets/splits/Split.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/Env.hpp"
#include "common/NetworkRequest.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/EmoteValue.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clipboard.hpp"
#include "util/NuulsUploader.hpp"
#include "util/Shortcut.hpp"
#include "util/StreamLink.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/TooltipWidget.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/QualityPopup.hpp"
#include "widgets/dialogs/SelectChannelDialog.hpp"
#include "widgets/dialogs/SelectChannelFiltersDialog.hpp"
#include "widgets/dialogs/TextInputDialog.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/DebugPopup.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/helper/SearchPopup.hpp"
#include "widgets/splits/ClosedSplits.hpp"
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

Split::Split(SplitContainer *parent)
    : Split(static_cast<QWidget *>(parent))
{
    this->container_ = parent;
}

Split::Split(QWidget *parent)
    : BaseWidget(parent)
    , container_(nullptr)
    , channel_(Channel::getEmpty())
    , vbox_(new QVBoxLayout(this))
    , header_(new SplitHeader(this))
    , view_(new ChannelView(this))
    , input_(new SplitInput(this))
    , overlay_(new SplitOverlay(this))
{
    this->setMouseTracking(true);
    this->view_->setPausable(true);
    this->view_->setFocusPolicy(Qt::FocusPolicy::NoFocus);

    this->vbox_->setSpacing(0);
    this->vbox_->setMargin(1);

    this->vbox_->addWidget(this->header_);
    this->vbox_->addWidget(this->view_, 1);
    this->vbox_->addWidget(this->input_);

    // Initialize chat widget-wide hotkeys
    // CTRL+W: Close Split
    createShortcut(this, "CTRL+W", &Split::deleteFromContainer);

    // CTRL+R: Change Channel
    createShortcut(this, "CTRL+R", &Split::changeChannel);

    // CTRL+F: Search
    createShortcut(this, "CTRL+F", &Split::showSearch);

    // F5: reload emotes
    createShortcut(this, "F5", &Split::reloadChannelAndSubscriberEmotes);

    // F10
    createShortcut(this, "F10", [] {
        auto *popup = new DebugPopup;
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->setWindowTitle("Chatterino - Debug popup");
        popup->show();
    });

    // xd
    // CreateShortcut(this, "ALT+SHIFT+RIGHT", &Split::doIncFlexX);
    // CreateShortcut(this, "ALT+SHIFT+LEFT", &Split::doDecFlexX);
    // CreateShortcut(this, "ALT+SHIFT+UP", &Split::doIncFlexY);
    // CreateShortcut(this, "ALT+SHIFT+DOWN", &Split::doDecFlexY);

    this->input_->ui_.textEdit->installEventFilter(parent);

    this->signalHolder_.managedConnect(
        getApp()->accounts->twitch.currentUserChanged, [this] {
            this->onAccountSelected();
        });
    this->onAccountSelected();

    this->view_->mouseDown.connect([this](QMouseEvent *) {
        this->giveFocus(Qt::MouseFocusReason);
    });
    this->view_->selectionChanged.connect([this]() {
        if (view_->hasSelection())
        {
            this->input_->clearSelection();
        }
    });

    this->view_->joinToChannel.connect([this](QString twitchChannel) {
        this->container_->appendNewSplit(false)->setChannel(
            getApp()->twitch.server->getOrAddChannel(twitchChannel));
    });

    this->input_->textChanged.connect([=](const QString &newText) {
        if (getSettings()->showEmptyInput)
        {
            return;
        }

        if (newText.length() == 0)
        {
            this->input_->hide();
        }
        else if (this->input_->isHidden())
        {
            this->input_->show();
        }
    });

    getSettings()->showEmptyInput.connect(
        [this](const bool &showEmptyInput, auto) {
            if (!showEmptyInput && this->input_->getInputText().length() == 0)
            {
                this->input_->hide();
            }
            else
            {
                this->input_->show();
            }
        },
        this->managedConnections_);

    this->header_->updateModerationModeIcon();
    this->overlay_->hide();

    this->setSizePolicy(QSizePolicy::MinimumExpanding,
                        QSizePolicy::MinimumExpanding);

    this->managedConnect(modifierStatusChanged, [this](Qt::KeyboardModifiers
                                                           status) {
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

    this->input_->ui_.textEdit->focused.connect([this] {
        this->focused.invoke();
    });
    this->input_->ui_.textEdit->focusLost.connect([this] {
        this->focusLost.invoke();
    });
    this->input_->ui_.textEdit->imagePasted.connect(
        [this](const QMimeData *source) {
            if (!getSettings()->imageUploaderEnabled)
                return;

            if (getSettings()->askOnImageUpload.getValue())
            {
                QMessageBox msgBox;
                msgBox.setText("Image upload");
                msgBox.setInformativeText(
                    "You are uploading an image to a 3rd party service not in "
                    "control of the chatterino team. You may not be able to "
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
        this->managedConnections_);
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

SplitContainer *Split::getContainer()
{
    return this->container_;
}

bool Split::isInContainer() const
{
    return this->container_ != nullptr;
}

void Split::setContainer(SplitContainer *container)
{
    this->container_ = container;
}

void Split::onAccountSelected()
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

    this->updateTooltipColor();
    this->signalHolder_.managedConnect(this->theme->updated, [this]() {
        this->updateTooltipColor();
    });
}

void Split::updateTooltipColor()
{
    QPalette dankPalette;
    dankPalette.setColor(QPalette::PlaceholderText,
                         this->theme->messages.textColors.chatPlaceholder);
    this->input_->ui_.textEdit->setPalette(dankPalette);
}

IndirectChannel Split::getIndirectChannel()
{
    return this->channel_;
}

ChannelPtr Split::getChannel()
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

    this->channelChanged.invoke();

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
    dialog->show();
    dialog->closed.connect([=] {
        if (dialog->hasSeletedChannel())
        {
            this->setChannel(dialog->getSelectedChannel());
            if (this->isInContainer())
            {
                this->container_->refreshTab();
            }
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

void Split::giveFocus(Qt::FocusReason reason)
{
    this->input_->ui_.textEdit->setFocus(reason);
}

bool Split::hasFocus() const
{
    return this->input_->ui_.textEdit->hasFocus();
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

    if (this->container_ != nullptr)
    {
        this->container_->resetMouseStatus();
    }
}

void Split::leaveEvent(QEvent *event)
{
    this->isMouseOver_ = false;

    this->overlay_->hide();

    TooltipWidget::instance()->hide();

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::focusInEvent(QFocusEvent *event)
{
    this->giveFocus(event->reason());
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
    if (this->container_)
    {
        this->container_->appendNewSplit(true);
    }
}

void Split::deleteFromContainer()
{
    if (this->container_)
    {
        this->container_->deleteSplit(this);
        auto *tab = this->getContainer()->getTab();
        tab->connect(tab, &QWidget::destroyed, [tab]() mutable {
            ClosedSplits::invalidateTab(tab);
        });
        ClosedSplits::push({this->getChannel()->getName(), tab});
    }
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
    ChannelPtr channel = this->getChannel();
    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        QDesktopServices::openUrl(
            "https://player.twitch.tv/?parent=twitch.tv&channel=" +
            twitchChannel->getName());
    }
}

void Split::openInStreamlink()
{
    try
    {
        openStreamlinkForChannel(this->getChannel()->getName());
    }
    catch (const Exception &ex)
    {
        qDebug() << "Error in doOpenStreamlink:" << ex.what();
    }
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
    QList<QListWidgetItem *> labelList;
    for (auto &x : labels)
    {
        auto label = formatListItemText(x);
        label->setForeground(this->theme->accent);
        labelList.append(label);
    }
    auto loadingLabel = new QLabel("Loading...");

    NetworkRequest::twitchRequest("https://tmi.twitch.tv/group/user/" +
                                  this->getChannel()->getName() + "/chatters")
        .caller(this)
        .onSuccess([=](auto result) -> Outcome {
            auto obj = result.parseJson();
            QJsonObject chattersObj = obj.value("chatters").toObject();

            loadingLabel->hide();
            for (int i = 0; i < jsonLabels.size(); i++)
            {
                auto currentCategory =
                    chattersObj.value(jsonLabels.at(i)).toArray();
                // If current category of chatters is empty, dont show this
                // category.
                if (currentCategory.empty())
                    continue;

                chattersList->addItem(labelList.at(i));
                foreach (const QJsonValue &v, currentCategory)
                {
                    chattersList->addItem(formatListItemText(v.toString()));
                }
                chattersList->addItem(new QListWidgetItem());
            }

            return Success;
        })
        .execute();

    searchBar->setPlaceholderText("Search User...");
    QObject::connect(searchBar, &QLineEdit::textEdited, this, [=]() {
        auto query = searchBar->text();
        if (!query.isEmpty())
        {
            auto results = chattersList->findItems(query, Qt::MatchStartsWith);
            chattersList->hide();
            resultList->clear();
            for (auto &item : results)
            {
                if (!labels.contains(item->text()))
                {
                    resultList->addItem(formatListItemText(item->text()));
                }
            }
            resultList->show();
        }
        else
        {
            resultList->hide();
            chattersList->show();
        }
    });

    QObject::connect(viewerDock, &QDockWidget::topLevelChanged, this, [=]() {
        viewerDock->setMinimumWidth(300);
    });

    auto listDoubleClick = [=](QString userName) {
        if (!labels.contains(userName) && !userName.isEmpty())
        {
            this->view_->showUserInfoPopup(userName);
        }
    };

    QObject::connect(chattersList, &QListWidget::doubleClicked, this, [=]() {
        listDoubleClick(chattersList->currentItem()->text());
    });

    QObject::connect(resultList, &QListWidget::doubleClicked, this, [=]() {
        listDoubleClick(resultList->currentItem()->text());
    });

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

void Split::showSearch()
{
    SearchPopup *popup = new SearchPopup(this);

    popup->setChannelFilters(this->view_->getFilterSet());
    popup->setAttribute(Qt::WA_DeleteOnClose);
    popup->setChannel(this->getChannel());
    popup->show();
}

void Split::reloadChannelAndSubscriberEmotes()
{
    getApp()->accounts->twitch.getCurrent()->loadEmotes();
    auto channel = this->getChannel();

    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        twitchChannel->refreshBTTVChannelEmotes(true);
        twitchChannel->refreshFFZChannelEmotes(true);
    }
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

}  // namespace chatterino
