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

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    QFont m_font;
    QVBoxLayout m_vbox;
    ChatWidgetHeader m_header;
    ChatWidgetView m_view;
    ChatWidgetInput m_input;

    Channel *m_channel = NULL;
};

#endif  // CHATWIDGET_H
