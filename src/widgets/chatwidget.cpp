#include "widgets/chatwidget.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "notebookpage.hpp"
#include "settingsmanager.hpp"
#include "widgets/textinputdialog.hpp"

#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QShortcut>
#include <QVBoxLayout>
#include <QFileInfo>
#include <QProcess>
#include <boost/signals2.hpp>

#include <functional>

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
    , channelManager(_channelManager)
    , completionManager(parent->completionManager)
    , channel(_channelManager.emptyChannel)
    , vbox(this)
    , header(this)
    , view(this)
    , input(this)
    , channelName("/chatWidgets/" + std::to_string(index++) + "/channelName")
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

    this->channelName.getValueChangedSignal().connect(
        std::bind(&ChatWidget::channelNameUpdated, this, std::placeholders::_1));

    this->channelNameUpdated(this->channelName.getValue());
}

ChatWidget::~ChatWidget()
{
    this->detachChannel();
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
    this->channel = _newChannel;

    // on new message
    this->messageAppendedConnection =
        this->channel->messageAppended.connect([this](SharedMessage &message) {
            SharedMessageRef deleted;

            auto messageRef = new MessageRef(message);

            if (this->messages.appendItem(SharedMessageRef(messageRef), deleted)) {
                qreal value = std::max(0.0, this->view.getScrollBar().getDesiredValue() - 1);

                this->view.getScrollBar().setDesiredValue(value, false);
            }
        });

    // on message removed
    this->messageRemovedConnection =
        this->channel->messageRemovedFromStart.connect([](SharedMessage &) {
            //
        });

    auto snapshot = this->channel->getMessageSnapshot();

    for (int i = 0; i < snapshot.getLength(); i++) {
        SharedMessageRef deleted;

        auto messageRef = new MessageRef(snapshot[i]);

        this->messages.appendItem(SharedMessageRef(messageRef), deleted);
    }
}

void ChatWidget::detachChannel()
{
    // on message added
    this->messageAppendedConnection.disconnect();

    // on message removed
    this->messageRemovedConnection.disconnect();
}

void ChatWidget::channelNameUpdated(const std::string &newChannelName)
{
    // remove current channel
    if (!this->channel->isEmpty()) {
        this->channelManager.removeChannel(this->channel->name);

        this->detachChannel();
    }

    // update messages
    this->messages.clear();

    if (newChannelName.empty()) {
        this->channel = this->channelManager.emptyChannel;
    } else {
        this->setChannel(this->channelManager.addChannel(QString::fromStdString(newChannelName)));
    }

    // update header
    this->header.updateChannelText();

    // update view
    this->layoutMessages(true);
}

LimitedQueueSnapshot<SharedMessageRef> ChatWidget::getMessagesSnapshot()
{
    return this->messages.getSnapshot();
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

        return true;
    }

    return false;
}

void ChatWidget::layoutMessages(bool forceUpdate)
{
    if (this->view.layoutMessages() || forceUpdate) {
        this->view.update();
    }
}

void ChatWidget::updateGifEmotes()
{
    this->view.updateGifEmotes();
}

void ChatWidget::giveFocus()
{
    this->input.textInput.setFocus();
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
    // Clear all stored messages in this chat widget
    this->messages.clear();

    // Layout chat widget messages, and force an update regardless if there are no messages
    this->layoutMessages(true);
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
    QString path = QString::fromStdString(settings.streamlinkPath.getValue());
    QFileInfo fileinfo = QFileInfo(path);
    // TODO(Confuseh): Add default checks for streamlink/livestreamer
    // TODO(Confuseh): Add quality switcher
    if (fileinfo.exists() && fileinfo.isExecutable()) {
        // works on leenux, idk whether it would work on whindows or mehOS
        QProcess::startDetached(path,
                                QStringList({"twitch.tv/" + QString::fromStdString(this->channelName.getValue()),
                                             "best"}));
    }
}

}  // namespace widgets
}  // namespace chatterino
