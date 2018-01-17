#include "widgets/split.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "twitch/twitchchannel.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "util/urlfetch.hpp"
#include "widgets/helper/searchpopup.hpp"
#include "widgets/helper/shortcut.hpp"
#include "widgets/qualitypopup.hpp"
#include "widgets/splitcontainer.hpp"
#include "widgets/textinputdialog.hpp"
#include "widgets/window.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QDockWidget>
#include <QDrag>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QProcess>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>
#include <boost/signals2.hpp>

#include <functional>
#include <random>

using namespace chatterino::messages;

namespace chatterino {
namespace widgets {

Split::Split(SplitContainer *parent, const std::string &_uuid)
    : BaseWidget(parent)
    , uuid(_uuid)
    , settingRoot(fS("/splits/{}", this->uuid))
    , channelName(fS("{}/channelName", this->settingRoot))
    , parentPage(*parent)
    , channel(singletons::ChannelManager::getInstance().emptyChannel)
    , vbox(this)
    , header(this)
    , view(this)
    , input(this)
    , flexSizeX(1)
    , flexSizeY(1)
    , moderationMode(false)
{
    this->setMouseTracking(true);

    this->vbox.setSpacing(0);
    this->vbox.setMargin(1);

    this->vbox.addWidget(&this->header);
    this->vbox.addWidget(&this->view, 1);
    this->vbox.addWidget(&this->input);

    // Initialize chat widget-wide hotkeys
    // CTRL+T: Create new split (Add page)
    CreateShortcut(this, "CTRL+T", &Split::doAddSplit);

    // CTRL+W: Close Split
    CreateShortcut(this, "CTRL+W", &Split::doCloseSplit);

    // CTRL+R: Change Channel
    CreateShortcut(this, "CTRL+R", &Split::doChangeChannel);

    // CTRL+F: Search
    CreateShortcut(this, "CTRL+F", &Split::doSearch);

    // xd
    // CreateShortcut(this, "ALT+SHIFT+RIGHT", &Split::doIncFlexX);
    // CreateShortcut(this, "ALT+SHIFT+LEFT", &Split::doDecFlexX);
    // CreateShortcut(this, "ALT+SHIFT+UP", &Split::doIncFlexY);
    // CreateShortcut(this, "ALT+SHIFT+DOWN", &Split::doDecFlexY);

    this->channelName.getValueChangedSignal().connect(
        std::bind(&Split::channelNameUpdated, this, std::placeholders::_1));

    this->channelNameUpdated(this->channelName.getValue());

    this->input.textInput.installEventFilter(parent);

    this->view.mouseDown.connect([this](QMouseEvent *) { this->giveFocus(Qt::MouseFocusReason); });
    this->view.selectionChanged.connect([this]() {
        if (view.hasSelection()) {
            this->input.clearSelection();
        }
    });

    this->input.textChanged.connect([this](const QString &newText) {
        if (!singletons::SettingManager::getInstance().hideEmptyInput) {
            return;
        }

        if (newText.length() == 0) {
            this->input.hide();
        } else if (this->input.isHidden()) {
            this->input.show();
        }
    });

    singletons::SettingManager::getInstance().hideEmptyInput.connect(
        [this](const bool &hideEmptyInput, auto) {
            if (hideEmptyInput && this->input.getInputText().length() == 0) {
                this->input.hide();
            } else {
                this->input.show();
            }
        });

    this->header.updateModerationModeIcon();
}

Split::~Split()
{
    this->channelNameUpdated("");
}

const std::string &Split::getUUID() const
{
    return this->uuid;
}

SharedChannel Split::getChannel() const
{
    return this->channel;
}

SharedChannel &Split::getChannelRef()
{
    return this->channel;
}

void Split::setChannel(SharedChannel _newChannel)
{
    this->view.setChannel(_newChannel);

    this->channel = _newChannel;

    this->channelChanged();
}

void Split::setFlexSizeX(double x)
{
    this->flexSizeX = x;
    this->parentPage.updateFlexValues();
}

double Split::getFlexSizeX()
{
    return this->flexSizeX;
}

void Split::setFlexSizeY(double y)
{
    this->flexSizeY = y;
    this->parentPage.updateFlexValues();
}

double Split::getFlexSizeY()
{
    return this->flexSizeY;
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

void Split::channelNameUpdated(const std::string &newChannelName)
{
    auto &cman = singletons::ChannelManager::getInstance();

    // remove current channel
    if (!this->channel->isEmpty()) {
        cman.removeTwitchChannel(this->channel->name);
    }

    // update messages
    if (newChannelName.empty()) {
        this->setChannel(cman.emptyChannel);
    } else {
        this->setChannel(cman.addTwitchChannel(QString::fromStdString(newChannelName)));
    }

    // update header
    this->header.updateChannelText();
}

bool Split::showChangeChannelPopup(const char *dialogTitle, bool empty)
{
    // create new input dialog and execute it
    TextInputDialog dialog(this);

    dialog.setWindowTitle(dialogTitle);

    if (!empty) {
        dialog.setText(QString::fromStdString(this->channelName));
    }

    if (dialog.exec() == QDialog::Accepted) {
        QString newChannelName = dialog.getText().trimmed();

        this->channelName = newChannelName.toStdString();
        this->parentPage.refreshTitle();

        return true;
    }

    return false;
}

void Split::layoutMessages()
{
    this->view.layoutMessages();
}

void Split::updateGifEmotes()
{
    qDebug() << "this shouldn't even exist";
    this->view.queueUpdate();
}

void Split::giveFocus(Qt::FocusReason reason)
{
    this->input.textInput.setFocus(reason);
}

bool Split::hasFocus() const
{
    return this->input.textInput.hasFocus();
}

void Split::paintEvent(QPaintEvent *)
{
    // color the background of the chat
    QPainter painter(this);

    painter.fillRect(this->rect(), this->themeManager.splits.background);
}

void Split::mouseMoveEvent(QMouseEvent *event)
{
    this->handleModifiers(event, event->modifiers());
}

void Split::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton && event->modifiers() & Qt::AltModifier) {
        this->drag();
    }
}

void Split::keyPressEvent(QKeyEvent *event)
{
    this->view.unsetCursor();
    this->handleModifiers(event, event->modifiers());
}

void Split::keyReleaseEvent(QKeyEvent *event)
{
    this->view.unsetCursor();
    this->handleModifiers(event, event->modifiers());
}

void Split::handleModifiers(QEvent *event, Qt::KeyboardModifiers modifiers)
{
    if (modifiers == Qt::AltModifier) {
        this->setCursor(Qt::SizeAllCursor);
        event->accept();
        //    } else if (modifiers == Qt::ControlModifier) {
        //        this->setCursor(Qt::SplitHCursor);
        //        event->accept();
    } else {
        this->setCursor(Qt::ArrowCursor);
    }
}

/// Slots
void Split::doAddSplit()
{
    SplitContainer *page = static_cast<SplitContainer *>(this->parentWidget());
    page->addChat(true);
}

void Split::doCloseSplit()
{
    SplitContainer *page = static_cast<SplitContainer *>(this->parentWidget());
    page->removeFromLayout(this);
}

void Split::doChangeChannel()
{
    this->showChangeChannelPopup("Change channel");
    auto popup = this->findChildren<QDockWidget *>();
    if (popup.size() && popup.at(0)->isVisible() && !popup.at(0)->isFloating()) {
        popup.at(0)->hide();
        doOpenViewerList();
    }
}

void Split::doPopup()
{
    Window &window = singletons::WindowManager::getInstance().createWindow();

    Split *split = new Split(static_cast<SplitContainer *>(window.getNotebook().getSelectedPage()),
                             this->uuid);

    window.getNotebook().getSelectedPage()->addToLayout(split);

    window.show();
}

void Split::doClearChat()
{
    this->view.clearMessages();
}

void Split::doOpenChannel()
{
    SharedChannel _channel = this->channel;
    twitch::TwitchChannel *tc = dynamic_cast<twitch::TwitchChannel *>(_channel.get());

    if (tc != nullptr) {
        QDesktopServices::openUrl("https://twitch.tv/" + tc->name);
    }
}

void Split::doOpenPopupPlayer()
{
    SharedChannel _channel = this->channel;
    twitch::TwitchChannel *tc = dynamic_cast<twitch::TwitchChannel *>(_channel.get());

    if (tc != nullptr) {
        QDesktopServices::openUrl("https://player.twitch.tv/?channel=" + tc->name);
    }
}

void Split::doOpenStreamlink()
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    QString preferredQuality = settings.preferredQuality;
    preferredQuality = preferredQuality.toLower();
    // TODO(Confuseh): Default streamlink paths
    QString path = settings.streamlinkPath;
    QString channel = QString::fromStdString(this->channelName.getValue());
    QFileInfo fileinfo = QFileInfo(path);

    if (path.isEmpty()) {
        debug::Log("[Split:doOpenStreamlink] No streamlink path selected in Settings");
        return;
    }

    if (!fileinfo.exists()) {
        debug::Log("[Split:doOpenStreamlink] Streamlink path ({}) is invalid, file does not exist",
                   path);
        return;
    }

    if (fileinfo.isDir() || !fileinfo.isExecutable()) {
        debug::Log("[Split:doOpenStreamlink] Streamlink path ({}) is invalid, it needs to point to "
                   "the streamlink executable",
                   path);
        return;
    }

    if (preferredQuality != "choose") {
        QStringList args = {"twitch.tv/" + channel};
        QString quality = "";
        QString exclude = "";
        if (preferredQuality == "high") {
            exclude = ">720p30";
            quality = "high,best";
        } else if (preferredQuality == "medium") {
            exclude = ">540p30";
            quality = "medium,best";
        } else if (preferredQuality == "low") {
            exclude = ">360p30";
            quality = "low,best";
        } else if (preferredQuality == "audio only") {
            quality = "audio,audio_only";
        } else {
            quality = "best";
        }
        if (quality != "")
            args << quality;
        if (exclude != "")
            args << "--stream-sorting-excludes" << exclude;
        QProcess::startDetached(path, args);
    } else {
        QProcess *p = new QProcess();
        // my god that signal though
        QObject::connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), this,
                         [path, channel, p](int) {
                             QString lastLine = QString(p->readAllStandardOutput());
                             lastLine = lastLine.trimmed().split('\n').last().trimmed();
                             if (lastLine.startsWith("Available streams: ")) {
                                 QStringList options;
                                 QStringList split =
                                     lastLine.right(lastLine.length() - 19).split(", ");

                                 for (int i = split.length() - 1; i >= 0; i--) {
                                     QString option = split.at(i);
                                     if (option.endsWith(" (worst)")) {
                                         options << option.left(option.length() - 8);
                                     } else if (option.endsWith(" (best)")) {
                                         options << option.left(option.length() - 7);
                                     } else {
                                         options << option;
                                     }
                                 }

                                 QualityPopup::showDialog(channel, path, options);
                             }
                         });
        p->start(path, {"twitch.tv/" + channel, "--default-stream=KKona"});
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

    auto accountPopup = new AccountPopupWidget(this->channel);
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
        label->setBackgroundColor(this->themeManager.splits.header.background);
        labelList.append(label);
    }
    auto loadingLabel = new QLabel("Loading...");

    util::twitch::get("https://tmi.twitch.tv/group/user/" + channel->name + "/chatters", this,
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
            doOpenAccountPopupWidget(accountPopup, chattersList->currentItem()->text());
        }
    });

    QObject::connect(resultList, &QListWidget::doubleClicked, this, [=]() {
        if (!labels.contains(resultList->currentItem()->text())) {
            doOpenAccountPopupWidget(accountPopup, resultList->currentItem()->text());
        }
    });

    dockVbox->addWidget(searchBar);
    dockVbox->addWidget(loadingLabel);
    dockVbox->addWidget(chattersList);
    dockVbox->addWidget(resultList);
    resultList->hide();

    multiWidget->setStyleSheet(this->themeManager.splits.input.styleSheet);
    multiWidget->setLayout(dockVbox);
    viewerDock->setWidget(multiWidget);
    viewerDock->show();
}

void Split::doOpenAccountPopupWidget(AccountPopupWidget *widget, QString user)
{
    widget->setName(user);
    widget->move(QCursor::pos());
    widget->updatePermissions();
    widget->show();
    widget->setFocus();
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

void Split::doIncFlexX()
{
    this->setFlexSizeX(this->getFlexSizeX() * 1.2);
}

void Split::doDecFlexX()
{
    this->setFlexSizeX(this->getFlexSizeX() * (1 / 1.2));
}

void Split::doIncFlexY()
{
    this->setFlexSizeY(this->getFlexSizeY() * 1.2);
}

void Split::doDecFlexY()
{
    this->setFlexSizeY(this->getFlexSizeY() * (1 / 1.2));
}

void Split::drag()
{
    auto container = dynamic_cast<SplitContainer *>(this->parentWidget());

    if (container != nullptr) {
        SplitContainer::isDraggingSplit = true;
        SplitContainer::draggingSplit = this;

        auto originalLocation = container->removeFromLayout(this);

        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;

        mimeData->setData("chatterino/split", "xD");

        drag->setMimeData(mimeData);

        Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

        if (dropAction == Qt::IgnoreAction) {
            container->addToLayout(this, originalLocation);
        }

        SplitContainer::isDraggingSplit = false;
    }
}
}  // namespace widgets
}  // namespace chatterino
