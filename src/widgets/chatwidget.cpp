#include "widgets/chatwidget.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "notebookpage.hpp"
#include "settingsmanager.hpp"
#include "twitch/twitchmessagebuilder.hpp"
#include "util/urlfetch.hpp"
#include "widgets/qualitypopup.hpp"
#include "widgets/textinputdialog.hpp"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDockWidget>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QListWidget>
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

namespace {

template <typename T>
inline void ezShortcut(ChatWidget *w, const char *key, T t)
{
    auto s = new QShortcut(QKeySequence(key), w);
    s->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(s, &QShortcut::activated, w, t);
}

}  // namespace

static int index = 0;

ChatWidget::ChatWidget(ChannelManager &_channelManager, NotebookPage *parent)
    : BaseWidget(parent)
    , parentPage(*parent)
    , channelManager(_channelManager)
    , completionManager(parent->completionManager)
    , channelName("/chatWidgets/" + std::to_string(index++) + "/channelName")
    , channel(_channelManager.emptyChannel)
    , vbox(this)
    , header(this)
    , view(_channelManager.getWindowManager(), this)
    , input(this, _channelManager.getEmoteManager(), _channelManager.getWindowManager())
{
    this->vbox.setSpacing(0);
    this->vbox.setMargin(1);

    this->vbox.addWidget(&this->header);
    this->vbox.addWidget(&this->view, 1);
    this->vbox.addWidget(&this->input);

    // Initialize chat widget-wide hotkeys
    // CTRL+T: Create new split (Add page)
    ezShortcut(this, "CTRL+T", &ChatWidget::doAddSplit);

    // CTRL+W: Close Split
    ezShortcut(this, "CTRL+W", &ChatWidget::doCloseSplit);

    // CTRL+R: Change Channel
    ezShortcut(this, "CTRL+R", &ChatWidget::doChangeChannel);

#ifndef NDEBUG
    // F12: Toggle message spawning
    ezShortcut(this, "ALT+Q", &ChatWidget::doToggleMessageSpawning);
#endif

    this->channelName.getValueChangedSignal().connect(
        std::bind(&ChatWidget::channelNameUpdated, this, std::placeholders::_1));

    this->channelNameUpdated(this->channelName.getValue());

    this->input.textInput.installEventFilter(parent);

    this->view.mouseDown.connect([this](QMouseEvent *) { this->giveFocus(Qt::MouseFocusReason); });
    this->view.selectionChanged.connect([this]() {
        if (view.hasSelection()) {
            this->input.clearSelection();
        }
    });

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ChatWidget::test);
    timer->start(1000);
}

ChatWidget::~ChatWidget()
{
    this->channelNameUpdated("");
}

std::shared_ptr<Channel> ChatWidget::getChannel() const
{
    return this->channel;
}

std::shared_ptr<Channel> &ChatWidget::getChannelRef()
{
    return this->channel;
}

void ChatWidget::setChannel(std::shared_ptr<Channel> _newChannel)
{
    this->view.setChannel(_newChannel);

    this->channel = _newChannel;

    this->channelChanged();
}

void ChatWidget::channelNameUpdated(const std::string &newChannelName)
{
    // remove current channel
    if (!this->channel->isEmpty()) {
        this->channelManager.removeTwitchChannel(this->channel->name);
    }

    // update messages
    if (newChannelName.empty()) {
        this->setChannel(this->channelManager.emptyChannel);
    } else {
        this->setChannel(
            this->channelManager.addTwitchChannel(QString::fromStdString(newChannelName)));
    }

    // update header
    this->header.updateChannelText();
}

bool ChatWidget::showChangeChannelPopup(const char *dialogTitle, bool empty)
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

void ChatWidget::layoutMessages()
{
    this->view.layoutMessages();
}

void ChatWidget::updateGifEmotes()
{
    this->view.updateGifEmotes();
}

void ChatWidget::giveFocus(Qt::FocusReason reason)
{
    this->input.textInput.setFocus(reason);
}

bool ChatWidget::hasFocus() const
{
    return this->input.textInput.hasFocus();
}

void ChatWidget::paintEvent(QPaintEvent *)
{
    // color the background of the chat
    QPainter painter(this);

    painter.fillRect(this->rect(), this->colorScheme.ChatBackground);
}

void ChatWidget::load(const boost::property_tree::ptree &tree)
{
    // load tab text
    try {
        this->channelName = tree.get<std::string>("channelName");
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree ChatWidget::save()
{
    boost::property_tree::ptree tree;

    tree.put("channelName", this->channelName.getValue());

    return tree;
}

/// Slots
void ChatWidget::doAddSplit()
{
    NotebookPage *page = static_cast<NotebookPage *>(this->parentWidget());
    page->addChat(true);
}

void ChatWidget::doCloseSplit()
{
    NotebookPage *page = static_cast<NotebookPage *>(this->parentWidget());
    page->removeFromLayout(this);
}

void ChatWidget::doChangeChannel()
{
    this->showChangeChannelPopup("Change channel");
    auto popup = this->findChildren<QDockWidget *>();
    if (popup.size() && popup.at(0)->isVisible() && !popup.at(0)->isFloating()) {
        popup.at(0)->hide();
        doOpenViewerList();
    }
}

void ChatWidget::doPopup()
{
    // TODO: Copy signals and stuff too
    auto widget =
        new ChatWidget(this->channelManager, static_cast<NotebookPage *>(this->parentWidget()));
    widget->channelName = this->channelName;
    widget->show();
}

void ChatWidget::doClearChat()
{
    view.clearMessages();
}

void ChatWidget::doOpenChannel()
{
    qDebug() << "[UNIMPLEMENTED] Open twitch.tv/"
             << QString::fromStdString(this->channelName.getValue());
}

void ChatWidget::doOpenPopupPlayer()
{
    qDebug() << "[UNIMPLEMENTED] Open twitch.tv/"
             << QString::fromStdString(this->channelName.getValue()) << "/popout";
}

void ChatWidget::doOpenStreamlink()
{
    SettingsManager &settings = SettingsManager::getInstance();
    QString preferredQuality =
        QString::fromStdString(settings.preferredQuality.getValue()).toLower();
    // TODO(Confuseh): Default streamlink paths
    QString path = QString::fromStdString(settings.streamlinkPath.getValue());
    QString channel = QString::fromStdString(this->channelName.getValue());
    QFileInfo fileinfo = QFileInfo(path);
    if (fileinfo.exists() && fileinfo.isExecutable()) {
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
                             [path, channel, p](int exitCode) {
                                 if (exitCode > 0) {
                                     return;
                                 }
                                 QString lastLine = QString(p->readAllStandardOutput());
                                 lastLine = lastLine.trimmed().split('\n').last();
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
            p->start(path, {"twitch.tv/" + channel});
        }
    }
}

void ChatWidget::doOpenViewerList()
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
        label->setBackgroundColor(this->colorScheme.ChatHeaderBackground);
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

    multiWidget->setStyleSheet(this->colorScheme.InputStyleSheet);
    multiWidget->setLayout(dockVbox);
    viewerDock->setWidget(multiWidget);
    viewerDock->show();
}

void ChatWidget::doOpenAccountPopupWidget(AccountPopupWidget *widget, QString user)
{
    widget->setName(user);
    widget->move(QCursor::pos());
    widget->updatePermissions();
    widget->show();
    widget->setFocus();
}

void ChatWidget::doCopy()
{
    QApplication::clipboard()->setText(this->view.getSelectedText());
}

static std::vector<std::string> usernameVariants = {
    "pajlada",                 //
    "trump",                   //
    "Chancu",                  //
    "pajaWoman",               //
    "fourtf",                  //
    "weneedmoreautisticbots",  //
    "fourtfbot",               //
    "pajbot",                  //
    "snusbot",                 //
};

static std::vector<std::string> messageVariants = {
    "hehe",                                         //
    "lol pajlada",                                  //
    "hehe BANNEDWORD",                              //
    "someone ordered pizza",                        //
    "for ice poseidon",                             //
    "and delivery guy said it is for enza denino",  //
    "!gn",                                          //
    "for my laptop",                                //
    "should I buy a Herschel backpack?",            //
};

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

void ChatWidget::test()
{
    if (this->testEnabled) {
        messages::MessageParseArgs args;

        auto message =
            new Communi::IrcPrivateMessage(this->channelManager.ircManager.readConnection.get());

        std::string text = *(select_randomly(messageVariants.begin(), messageVariants.end()));
        std::string username = *(select_randomly(usernameVariants.begin(), usernameVariants.end()));
        std::string usernameString = username + "!" + username + "@" + username;

        QStringList params{"#pajlada", text.c_str()};

        qDebug() << params;

        message->setParameters(params);

        message->setPrefix(usernameString.c_str());

        auto twitchChannel = std::dynamic_pointer_cast<twitch::TwitchChannel>(this->channel);

        twitch::TwitchMessageBuilder builder(
            twitchChannel.get(), this->channelManager.ircManager.resources,
            this->channelManager.emoteManager, this->channelManager.windowManager, message, args);

        twitchChannel->addMessage(builder.parse());
    }
}

void ChatWidget::doToggleMessageSpawning()
{
    this->testEnabled = !this->testEnabled;
}

}  // namespace widgets
}  // namespace chatterino
