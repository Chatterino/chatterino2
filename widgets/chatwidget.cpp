#include "widgets/chatwidget.h"
#include "channels.h"
#include "colorscheme.h"
#include "settings.h"
#include "widgets/textinputdialog.h"

#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QVBoxLayout>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , messages()
    , channel(Channels::getEmpty())
    , channelName(QString())
    , vbox(this)
    , header(this)
    , view(this)
    , input(this)
{
    this->vbox.setSpacing(0);
    this->vbox.setMargin(1);

    this->vbox.addWidget(&header);
    this->vbox.addWidget(&view, 1);
    this->vbox.addWidget(&input);
}

ChatWidget::~ChatWidget()
{
}

void
ChatWidget::setChannelName(const QString &name)
{
    QString channel = name.trimmed();

    if (QString::compare(channel, this->channelName, Qt::CaseInsensitive) ==
        0) {
        this->channelName = channel;
        this->header.updateChannelText();
        return;
    }

    if (!this->channelName.isEmpty()) {
        Channels::removeChannel(this->channelName);

        this->messageAppendedConnection.disconnect();
        this->messageRemovedConnection.disconnect();
    }

    this->channelName = channel;
    this->header.updateChannelText();

    this->view.layoutMessages();

    messages.clear();

    if (channel.isEmpty()) {
        this->channel = NULL;

    } else {
        this->channel = Channels::addChannel(channel);

        this->messageAppendedConnection =
            this->channel.get()->messageAppended.connect([this](
                std::shared_ptr<messages::Message> &message) {

                std::shared_ptr<messages::MessageRef> deleted;

                auto messageRef = new messages::MessageRef(message);

                this->messages.appendItem(
                    std::shared_ptr<messages::MessageRef>(messageRef), deleted);
            });

        this->messageRemovedConnection =
            this->channel.get()->messageRemovedFromStart.connect(
                [this](std::shared_ptr<messages::Message> &) {});

        auto snapshot = this->channel.get()->getMessageSnapshot();

        for (int i = 0; i < snapshot.getLength(); i++) {
            std::shared_ptr<messages::MessageRef> deleted;

            auto messageRef = new messages::MessageRef(snapshot[i]);

            this->messages.appendItem(
                std::shared_ptr<messages::MessageRef>(messageRef), deleted);
        }
    }

    this->view.layoutMessages();
    this->view.update();
}

void
ChatWidget::showChangeChannelPopup()
{
    TextInputDialog dialog(this);

    dialog.setText(this->channelName);

    if (dialog.exec() == QDialog::Accepted) {
        setChannelName(dialog.getText());
    }
}

void
ChatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), ColorScheme::getInstance().ChatBackground);
}

void
ChatWidget::load(const boost::property_tree::ptree &tree)
{
    // Load tab text
    try {
        this->setChannelName(
            QString::fromStdString(tree.get<std::string>("channelName")));
    } catch (boost::property_tree::ptree_error) {
    }
}

boost::property_tree::ptree
ChatWidget::save()
{
    boost::property_tree::ptree tree;

    tree.put("channelName", this->getChannelName().toStdString());

    return tree;
}

}  // namespace widgets
}  // namespace chatterino
