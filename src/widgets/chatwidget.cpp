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
#include <boost/signals2.hpp>

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

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , channel(ChannelManager::getInstance().getEmpty())
    , vbox(this)
    , header(this)
    , view(this)
    , input(this)
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

const QString &ChatWidget::getChannelName() const
{
    return this->channelName;
}

void ChatWidget::setChannelName(const QString &_newChannelName)
{
    QString newChannelName = _newChannelName.trimmed();

    // return if channel name is the same
    if (QString::compare(newChannelName, this->channelName, Qt::CaseInsensitive) == 0) {
        this->channelName = newChannelName;
        this->header.updateChannelText();

        return;
    }

    // remove current channel
    if (!this->channelName.isEmpty()) {
        ChannelManager::getInstance().removeChannel(this->channelName);

        this->detachChannel();
    }

    // update members
    this->channelName = newChannelName;

    // update messages
    this->messages.clear();

    if (newChannelName.isEmpty()) {
        this->channel = nullptr;
    } else {
        this->setChannel(ChannelManager::getInstance().addChannel(newChannelName));
    }

    // update header
    this->header.updateChannelText();

    // update view
    this->view.layoutMessages();
    this->view.update();
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

    for (int i = 0; i < snapshot.getSize(); i++) {
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

LimitedQueueSnapshot<SharedMessageRef> ChatWidget::getMessagesSnapshot()
{
    return this->messages.getSnapshot();
}

void ChatWidget::showChangeChannelPopup()
{
    // create new input dialog and execute it
    TextInputDialog dialog(this);

    dialog.setText(this->channelName);

    if (dialog.exec() == QDialog::Accepted) {
        setChannelName(dialog.getText());
    }
}

void ChatWidget::layoutMessages()
{
    if (this->view.layoutMessages()) {
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

    painter.fillRect(this->rect(), ColorScheme::getInstance().ChatBackground);
}

void ChatWidget::load(const boost::property_tree::ptree &tree)
{
    // load tab text
    try {
        this->setChannelName(QString::fromStdString(tree.get<std::string>("channelName")));
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree ChatWidget::save()
{
    boost::property_tree::ptree tree;

    tree.put("channelName", this->getChannelName().toStdString());

    return tree;
}

/// Slots
void ChatWidget::doAddSplit()
{
    NotebookPage *page = static_cast<NotebookPage *>(this->parentWidget());
    page->addChat();
}

void ChatWidget::doCloseSplit()
{
    NotebookPage *page = static_cast<NotebookPage *>(this->parentWidget());
    page->removeFromLayout(this);
}

void ChatWidget::doChangeChannel()
{
    this->showChangeChannelPopup();
}

void ChatWidget::doPopup()
{
    // TODO: Copy signals and stuff too
    auto widget = new ChatWidget();
    widget->setChannelName(this->getChannelName());
    widget->show();
}

void ChatWidget::doClearChat()
{
    qDebug() << "[UNIMPLEMENTED] Clear chat";
}

void ChatWidget::doOpenChannel()
{
    qDebug() << "[UNIMPLEMENTED] Open twitch.tv/" << this->getChannelName();
}

void ChatWidget::doOpenPopupPlayer()
{
    qDebug() << "[UNIMPLEMENTED] Open twitch.tv/" << this->getChannelName() << "/popout";
}

}  // namespace widgets
}  // namespace chatterino
