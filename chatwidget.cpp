#include "chatwidget.h"
#include "channels.h"
#include "colorscheme.h"
#include "textinputdialog.h"

#include <QFont>
#include <QFontDatabase>
#include <QPainter>
#include <QVBoxLayout>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
    , m_channel(NULL)
    , m_channelName(QString())
    , m_vbox(this)
    , m_header(this)
    , m_view(this)
    , m_input()
{
    m_vbox.setSpacing(0);
    m_vbox.setMargin(1);

    m_vbox.addWidget(&m_header);
    m_vbox.addWidget(&m_view);
    m_vbox.addWidget(&m_input);
}

ChatWidget::~ChatWidget()
{
}

void
ChatWidget::setChannelName(const QString &name)
{
    QString channel = name.trimmed();

    if (QString::compare(channel, m_channelName, Qt::CaseInsensitive) == 0) {
        m_channelName = channel;
        m_header.updateChannelText();
        return;
    }

    m_channelName = channel;
    m_header.updateChannelText();

    m_view.layoutMessages();

    if (!m_channelName.isEmpty()) {
        Channels::removeChannel(m_channelName);
    }

    if (channel.isEmpty()) {
        m_channel = NULL;
    } else {
        m_channel = Channels::addChannel(channel);
    }
}

void
ChatWidget::showChangeChannelPopup()
{
    TextInputDialog dialog(this);

    dialog.setText(m_channelName);

    if (dialog.exec() == QDialog::Accepted) {
        setChannelName(dialog.text());
    }
}

void
ChatWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(rect(), ColorScheme::instance().ChatBackground);
}
