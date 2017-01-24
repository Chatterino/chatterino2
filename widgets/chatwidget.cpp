#include "widgets/chatwidget.h"
#include "channels.h"
#include "colorscheme.h"
#include "settings.h"
#include "widgets/textinputdialog.h"

#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , channel(NULL)
    , channelName(QString())
    , vbox(this)
    , header(this)
    , view(this)
    , input()
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

    this->channelName = channel;
    this->header.updateChannelText();

    this->view.layoutMessages();

    if (!this->channelName.isEmpty()) {
        Channels::removeChannel(this->channelName);
    }

    if (channel.isEmpty()) {
        this->channel = NULL;
    } else {
        this->channel = Channels::addChannel(channel);
    }
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
}
}
