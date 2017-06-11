#include "widgets/chatwidget.h"
#include "channelmanager.h"
#include "colorscheme.h"
#include "notebookpage.h"
#include "settingsmanager.h"
#include "widgets/textinputdialog.h"

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

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , _messages()
    , _channel(ChannelManager::getInstance().getEmpty())
    , _channelName(QString())
    , _vbox(this)
    , _header(this)
    , _view(this)
    , _input(this)
{
    this->_vbox.setSpacing(0);
    this->_vbox.setMargin(1);

    this->_vbox.addWidget(&_header);
    this->_vbox.addWidget(&_view, 1);
    this->_vbox.addWidget(&_input);

    // Initialize widget-wide hotkeys
    // CTRL+T: Create new split (Add page)
    {
        auto s = new QShortcut(QKeySequence("CTRL+T"), this);
        s->setContext(Qt::WidgetWithChildrenShortcut);
        connect(s, &QShortcut::activated, this, &ChatWidget::doAddSplit);
    }

    // CTRL+W: Close Split
    {
        auto s = new QShortcut(QKeySequence("CTRL+W"), this);
        s->setContext(Qt::WidgetWithChildrenShortcut);
        connect(s, &QShortcut::activated, this, &ChatWidget::doCloseSplit);
    }

    // CTRL+R: Change Channel
    {
        auto s = new QShortcut(QKeySequence("CTRL+R"), this);
        s->setContext(Qt::WidgetWithChildrenShortcut);
        connect(s, &QShortcut::activated, this, &ChatWidget::doChangeChannel);
    }
}

ChatWidget::~ChatWidget()
{
    detachChannel();
}

std::shared_ptr<Channel> ChatWidget::getChannel() const
{
    return _channel;
}

std::shared_ptr<Channel> &ChatWidget::getChannelRef()
{
    return _channel;
}

const QString &ChatWidget::getChannelName() const
{
    return _channelName;
}

void ChatWidget::setChannelName(const QString &name)
{
    QString channelName = name.trimmed();

    // return if channel name is the same
    if (QString::compare(channelName, _channelName, Qt::CaseInsensitive) == 0) {
        _channelName = channelName;
        _header.updateChannelText();

        return;
    }

    // remove current channel
    if (!_channelName.isEmpty()) {
        ChannelManager::getInstance().removeChannel(_channelName);

        detachChannel();
    }

    // update members
    _channelName = channelName;

    // update messages
    _messages.clear();

    printf("Set channel name xD %s\n", qPrintable(name));

    if (channelName.isEmpty()) {
        _channel = nullptr;
    } else {
        _channel = ChannelManager::getInstance().addChannel(channelName);
        printf("Created channel FeelsGoodMan %p\n", _channel.get());

        attachChannel(_channel);
    }

    // update header
    _header.updateChannelText();

    // update view
    _view.layoutMessages();
    _view.update();
}

void ChatWidget::attachChannel(SharedChannel channel)
{
    // on new message
    _messageAppendedConnection = channel->messageAppended.connect([this](SharedMessage &message) {
        SharedMessageRef deleted;

        auto messageRef = new MessageRef(message);

        if (_messages.appendItem(SharedMessageRef(messageRef), deleted)) {
            qreal value = std::max(0.0, _view.getScrollbar()->getDesiredValue() - 1);

            _view.getScrollbar()->setDesiredValue(value, false);
        }
    });

    // on message removed
    _messageRemovedConnection = _channel->messageRemovedFromStart.connect([](SharedMessage &) {
        //
    });

    auto snapshot = _channel.get()->getMessageSnapshot();

    for (int i = 0; i < snapshot.getSize(); i++) {
        SharedMessageRef deleted;

        auto messageRef = new MessageRef(snapshot[i]);

        _messages.appendItem(SharedMessageRef(messageRef), deleted);
    }
}

void ChatWidget::detachChannel()
{
    // on message added
    _messageAppendedConnection.disconnect();

    // on message removed
    _messageRemovedConnection.disconnect();
}

LimitedQueueSnapshot<SharedMessageRef> ChatWidget::getMessagesSnapshot()
{
    return _messages.getSnapshot();
}

void ChatWidget::showChangeChannelPopup()
{
    // create new input dialog and execute it
    TextInputDialog dialog(this);

    dialog.setText(_channelName);

    if (dialog.exec() == QDialog::Accepted) {
        setChannelName(dialog.getText());
    }
}

void ChatWidget::layoutMessages()
{
    if (_view.layoutMessages()) {
        _view.update();
    }
}

void ChatWidget::updateGifEmotes()
{
    _view.updateGifEmotes();
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
//
void ChatWidget::doAddSplit()
{
    NotebookPage *page = static_cast<NotebookPage *>(this->parentWidget());
    page->addChat();
}

void ChatWidget::doCloseSplit()
{
    qDebug() << "Close split for" << this->getChannelName();
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
    qDebug() << "[UNIMPLEMENTED]: Clear chat";
}

void ChatWidget::doOpenChannel()
{
    qDebug() << "[UNIMPLEMENTED]: Open twitch.tv/" << this->getChannelName();
}

void ChatWidget::doOpenPopupPlayer()
{
    qDebug() << "[UNIMPLEMENTED]: Open twitch.tv/" << this->getChannelName() << "/popout";
}

}  // namespace widgets
}  // namespace chatterino
