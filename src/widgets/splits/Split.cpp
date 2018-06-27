#include "widgets/splits/Split.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "common/UrlFetch.hpp"
#include "providers/twitch/EmoteValue.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "providers/twitch/TwitchMessageBuilder.hpp"
#include "providers/twitch/TwitchServer.hpp"
#include "singletons/SettingsManager.hpp"
#include "singletons/ThemeManager.hpp"
#include "singletons/WindowManager.hpp"
#include "util/StreamLink.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/QualityPopup.hpp"
#include "widgets/dialogs/SelectChannelDialog.hpp"
#include "widgets/dialogs/TextInputDialog.hpp"
#include "widgets/dialogs/UserInfoPopup.hpp"
#include "widgets/helper/DebugPopup.hpp"
#include "widgets/helper/SearchPopup.hpp"
#include "widgets/helper/Shortcut.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/splits/SplitOverlay.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDrag>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QVBoxLayout>

#include <functional>
#include <random>

namespace chatterino {

pajlada::Signals::Signal<Qt::KeyboardModifiers> Split::modifierStatusChanged;
Qt::KeyboardModifiers Split::modifierStatus = Qt::NoModifier;

Split::Split(SplitContainer *parent)
    : Split(static_cast<QWidget *>(parent))
{
    this->container = parent;
}

Split::Split(QWidget *parent)
    : BaseWidget(parent)
    , container(nullptr)
    , channel(Channel::getEmpty())
    , vbox(this)
    , header(this)
    , view(this)
    , input(this)
    , overlay(new SplitOverlay(this))
{
    auto app = getApp();

    this->setMouseTracking(true);

    this->vbox.setSpacing(0);
    this->vbox.setMargin(1);

    this->vbox.addWidget(&this->header);
    this->vbox.addWidget(&this->view, 1);
    this->vbox.addWidget(&this->input);

    // Initialize chat widget-wide hotkeys
    // CTRL+W: Close Split
    CreateShortcut(this, "CTRL+W", &Split::doCloseSplit);

    // CTRL+R: Change Channel
    CreateShortcut(this, "CTRL+R", &Split::doChangeChannel);

    // CTRL+F: Search
    CreateShortcut(this, "CTRL+F", &Split::doSearch);

    // F12
    CreateShortcut(this, "F10", [] {
        auto *popup = new DebugPopup;
        popup->setAttribute(Qt::WA_DeleteOnClose);
        popup->show();
    });

    // xd
    // CreateShortcut(this, "ALT+SHIFT+RIGHT", &Split::doIncFlexX);
    // CreateShortcut(this, "ALT+SHIFT+LEFT", &Split::doDecFlexX);
    // CreateShortcut(this, "ALT+SHIFT+UP", &Split::doIncFlexY);
    // CreateShortcut(this, "ALT+SHIFT+DOWN", &Split::doDecFlexY);

    this->input.ui_.textEdit->installEventFilter(parent);

    this->view.mouseDown.connect([this](QMouseEvent *) {
        //
        this->giveFocus(Qt::MouseFocusReason);
    });
    this->view.selectionChanged.connect([this]() {
        if (view.hasSelection()) {
            this->input.clearSelection();
        }
    });

    this->input.textChanged.connect([=](const QString &newText) {
        if (!app->settings->hideEmptyInput) {
            return;
        }

        if (newText.length() == 0) {
            this->input.hide();
        } else if (this->input.isHidden()) {
            this->input.show();
        }
    });

    app->settings->hideEmptyInput.connect(
        [this](const bool &hideEmptyInput, auto) {
            if (hideEmptyInput && this->input.getInputText().length() == 0) {
                this->input.hide();
            } else {
                this->input.show();
            }
        },
        this->managedConnections);

    this->header.updateModerationModeIcon();
    this->overlay->hide();

    this->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    this->managedConnect(modifierStatusChanged, [this](Qt::KeyboardModifiers status) {
        if ((status == showSplitOverlayModifiers /*|| status == showAddSplitRegions*/) &&
            this->isMouseOver) {
            this->overlay->show();
        } else {
            this->overlay->hide();
        }
    });

    this->input.ui_.textEdit->focused.connect([this] { this->focused.invoke(); });
    this->input.ui_.textEdit->focusLost.connect([this] { this->focusLost.invoke(); });
}

Split::~Split()
{
    this->usermodeChangedConnection.disconnect();
    this->roomModeChangedConnection.disconnect();
    this->channelIDChangedConnection.disconnect();
    this->indirectChannelChangedConnection.disconnect();
}

ChannelView &Split::getChannelView()
{
    return this->view;
}

SplitContainer *Split::getContainer()
{
    return this->container;
}

bool Split::isInContainer() const
{
    return this->container != nullptr;
}

void Split::setContainer(SplitContainer *_container)
{
    this->container = _container;
}

IndirectChannel Split::getIndirectChannel()
{
    return this->channel;
}

ChannelPtr Split::getChannel()
{
    return this->channel.get();
}

void Split::setChannel(IndirectChannel newChannel)
{
    this->channel = newChannel;

    this->view.setChannel(newChannel.get());

    this->usermodeChangedConnection.disconnect();
    this->roomModeChangedConnection.disconnect();
    this->indirectChannelChangedConnection.disconnect();

    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(newChannel.get().get());

    if (tc != nullptr) {
        this->usermodeChangedConnection =
            tc->userStateChanged.connect([this] { this->header.updateModerationModeIcon(); });

        this->roomModeChangedConnection =
            tc->roomModesChanged.connect([this] { this->header.updateModes(); });
    }

    this->indirectChannelChangedConnection = newChannel.getChannelChanged().connect([this] {  //
        QTimer::singleShot(0, [this] { this->setChannel(this->channel); });
    });

    this->header.updateModerationModeIcon();
    this->header.updateChannelText();
    this->header.updateModes();

    this->channelChanged.invoke();
}

void Split::setModerationMode(bool value)
{
    if (value != this->moderationMode) {
        this->moderationMode = value;
        this->header.updateModerationModeIcon();
        this->view.layoutMessages();
    }
}

bool Split::getModerationMode() const
{
    return this->moderationMode;
}

void Split::showChangeChannelPopup(const char *dialogTitle, bool empty,
                                   std::function<void(bool)> callback)
{
    if (this->selectChannelDialog.hasElement()) {
        this->selectChannelDialog->raise();

        return;
    }

    SelectChannelDialog *dialog = new SelectChannelDialog(this);
    if (!empty) {
        dialog->setSelectedChannel(this->getIndirectChannel());
    }
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->closed.connect([=] {
        if (dialog->hasSeletedChannel()) {
            this->setChannel(dialog->getSelectedChannel());
            if (this->isInContainer()) {
                this->container->refreshTabTitle();
            }
        }

        callback(dialog->hasSeletedChannel());
        this->selectChannelDialog = nullptr;
    });
    this->selectChannelDialog = dialog;
}

void Split::layoutMessages()
{
    this->view.layoutMessages();
}

void Split::updateGifEmotes()
{
    this->view.queueUpdate();
}

void Split::updateLastReadMessage()
{
    this->view.updateLastReadMessage();
}

void Split::giveFocus(Qt::FocusReason reason)
{
    this->input.ui_.textEdit->setFocus(reason);
}

bool Split::hasFocus() const
{
    return this->input.ui_.textEdit->hasFocus();
}

void Split::paintEvent(QPaintEvent *)
{
    // color the background of the chat
    QPainter painter(this);

    painter.fillRect(this->rect(), this->themeManager->splits.background);
}

void Split::mouseMoveEvent(QMouseEvent *event)
{
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::keyPressEvent(QKeyEvent *event)
{
    this->view.unsetCursor();
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::keyReleaseEvent(QKeyEvent *event)
{
    this->view.unsetCursor();
    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::resizeEvent(QResizeEvent *event)
{
    BaseWidget::resizeEvent(event);

    this->overlay->setGeometry(this->rect());
}

void Split::enterEvent(QEvent *event)
{
    this->isMouseOver = true;

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());

    if (modifierStatus == showSplitOverlayModifiers /*|| modifierStatus == showAddSplitRegions*/) {
        this->overlay->show();
    }

    if (this->container != nullptr) {
        this->container->resetMouseStatus();
    }
}

void Split::leaveEvent(QEvent *event)
{
    this->isMouseOver = false;

    this->overlay->hide();

    this->handleModifiers(QGuiApplication::queryKeyboardModifiers());
}

void Split::focusInEvent(QFocusEvent *event)
{
    this->giveFocus(event->reason());
}

void Split::handleModifiers(Qt::KeyboardModifiers modifiers)
{
    if (modifierStatus != modifiers) {
        modifierStatus = modifiers;
        modifierStatusChanged.invoke(modifiers);
    }
}

/// Slots
void Split::doAddSplit()
{
    if (this->container) {
        this->container->appendNewSplit(true);
    }
}

void Split::doCloseSplit()
{
    if (this->container) {
        this->container->deleteSplit(this);
    }
}

void Split::doChangeChannel()
{
    this->showChangeChannelPopup("Change channel", false, [](bool) {});

    auto popup = this->findChildren<QDockWidget *>();
    if (popup.size() && popup.at(0)->isVisible() && !popup.at(0)->isFloating()) {
        popup.at(0)->hide();
        doOpenViewerList();
    }
}

void Split::doPopup()
{
    auto app = getApp();
    Window &window = app->windows->createWindow(Window::Popup);

    Split *split =
        new Split(static_cast<SplitContainer *>(window.getNotebook().getOrAddSelectedPage()));

    split->setChannel(this->getIndirectChannel());
    window.getNotebook().getOrAddSelectedPage()->appendSplit(split);

    window.show();
}

void Split::doClearChat()
{
    this->view.clearMessages();
}

void Split::doOpenChannel()
{
    ChannelPtr _channel = this->getChannel();
    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

    if (tc != nullptr) {
        QDesktopServices::openUrl("https://twitch.tv/" + tc->name);
    }
}

void Split::doOpenPopupPlayer()
{
    ChannelPtr _channel = this->getChannel();
    TwitchChannel *tc = dynamic_cast<TwitchChannel *>(_channel.get());

    if (tc != nullptr) {
        QDesktopServices::openUrl("https://player.twitch.tv/?channel=" + tc->name);
    }
}

void Split::doOpenStreamlink()
{
    try {
        Start(this->getChannel()->name);
    } catch (const Exception &ex) {
        Log("Error in doOpenStreamlink: {}", ex.what());
    }
}

void Split::doOpenViewerList()
{
    auto viewerDock = new QDockWidget("Viewer List", this);
    viewerDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    viewerDock->setFeatures(QDockWidget::DockWidgetVerticalTitleBar |
                            QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetFloatable);
    viewerDock->resize(0.5 * this->width(),
                       this->height() - this->header.height() - this->input.height());
    viewerDock->move(0, this->header.height());

    auto multiWidget = new QWidget(viewerDock);
    auto dockVbox = new QVBoxLayout(viewerDock);
    auto searchBar = new QLineEdit(viewerDock);

    auto chattersList = new QListWidget();
    auto resultList = new QListWidget();

    static QStringList labels = {"Moderators", "Staff", "Admins", "Global Moderators", "Viewers"};
    static QStringList jsonLabels = {"moderators", "staff", "admins", "global_mods", "viewers"};
    QList<QListWidgetItem *> labelList;
    for (auto &x : labels) {
        auto label = new QListWidgetItem(x);
        label->setBackgroundColor(this->themeManager->splits.header.background);
        labelList.append(label);
    }
    auto loadingLabel = new QLabel("Loading...");

    if (!this->getChannel()->name.startsWith("chatrooms:")) {
        twitchApiGet("https://tmi.twitch.tv/group/user/" + this->getChannel()->name + "/chatters", this,
                     [=](QJsonObject obj) {
                         QJsonObject chattersObj = obj.value("chatters").toObject();

                         loadingLabel->hide();
                         for (int i = 0; i < jsonLabels.size(); i++) {
                             chattersList->addItem(labelList.at(i));
                             foreach (const QJsonValue &v,
                                      chattersObj.value(jsonLabels.at(i)).toArray())
                                 chattersList->addItem(v.toString());
                         }
                     });
    } else {
        loadingLabel->hide();
    }

    searchBar->setPlaceholderText("Search User...");
    QObject::connect(searchBar, &QLineEdit::textEdited, this, [=]() {
        auto query = searchBar->text();
        if (!query.isEmpty()) {
            auto results = chattersList->findItems(query, Qt::MatchStartsWith);
            chattersList->hide();
            resultList->clear();
            for (auto &item : results) {
                if (!labels.contains(item->text()))
                    resultList->addItem(item->text());
            }
            resultList->show();
        } else {
            resultList->hide();
            chattersList->show();
        }
    });

    QObject::connect(viewerDock, &QDockWidget::topLevelChanged, this,
                     [=]() { viewerDock->setMinimumWidth(300); });

    QObject::connect(chattersList, &QListWidget::doubleClicked, this, [=]() {
        if (!labels.contains(chattersList->currentItem()->text())) {
            doOpenUserInfoPopup(chattersList->currentItem()->text());
        }
    });

    QObject::connect(resultList, &QListWidget::doubleClicked, this, [=]() {
        if (!labels.contains(resultList->currentItem()->text())) {
            doOpenUserInfoPopup(resultList->currentItem()->text());
        }
    });

    dockVbox->addWidget(searchBar);
    dockVbox->addWidget(loadingLabel);
    dockVbox->addWidget(chattersList);
    dockVbox->addWidget(resultList);
    resultList->hide();

    multiWidget->setStyleSheet(this->themeManager->splits.input.styleSheet);
    multiWidget->setLayout(dockVbox);
    viewerDock->setWidget(multiWidget);
    viewerDock->show();
}

void Split::doOpenUserInfoPopup(const QString &user)
{
    auto *userPopup = new UserInfoPopup;
    userPopup->setData(user, this->getChannel());
    userPopup->setAttribute(Qt::WA_DeleteOnClose);
    userPopup->move(QCursor::pos() -
                    QPoint(int(150 * this->getScale()), int(70 * this->getScale())));
    userPopup->show();
}

void Split::doCopy()
{
    QApplication::clipboard()->setText(this->view.getSelectedText());
}

void Split::doSearch()
{
    SearchPopup *popup = new SearchPopup();

    popup->setChannel(this->getChannel());
    popup->show();
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
    auto container = dynamic_cast<SplitContainer *>(this->parentWidget());

    if (container != nullptr) {
        SplitContainer::isDraggingSplit = true;
        SplitContainer::draggingSplit = this;

        auto originalLocation = container->releaseSplit(this);

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setData("chatterino/split", "xD");

        drag->setMimeData(mimeData);

        Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

        if (dropAction == Qt::IgnoreAction) {
            container->insertSplit(this,
                                   originalLocation);  // SplitContainer::dragOriginalPosition);
        }

        SplitContainer::isDraggingSplit = false;
    }
}

}  // namespace chatterino
