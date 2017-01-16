#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include "QFont"
#include "QVBoxLayout"
#include "channel.h"
#include "chatwidgetheader.h"
#include "chatwidgetinput.h"
#include "chatwidgetview.h"

class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    ChatWidget(QWidget *parent = 0);
    ~ChatWidget();

    ChatWidgetView &
    view()
    {
        return m_view;
    }

    Channel *
    channel() const
    {
        return m_channel;
    }

    const QString &
    channelName() const
    {
        return m_channelName;
    }

    void setChannelName(const QString &name);

    void showChangeChannelPopup();

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    Channel *m_channel;
    QString m_channelName;

    QFont m_font;
    QVBoxLayout m_vbox;
    ChatWidgetHeader m_header;
    ChatWidgetView m_view;
    ChatWidgetInput m_input;
};

#endif  // CHATWIDGET_H
