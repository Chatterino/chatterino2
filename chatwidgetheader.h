#ifndef CHATWIDGETHEADER_H
#define CHATWIDGETHEADER_H

#include "chatwidgetheaderbutton.h"
#include "signallabel.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QWidget>

class ChatWidget;

class ChatWidgetHeader : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetHeader(ChatWidget *parent);

    ChatWidget *
    getChatWidget()
    {
        return chatWidget;
    }

    void updateColors();
    void updateChannelText();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    ChatWidget *chatWidget;

    QPoint dragStart;
    bool dragging;

    QHBoxLayout hbox;

    ChatWidgetHeaderButton leftLabel;
    SignalLabel middleLabel;
    ChatWidgetHeaderButton rightLabel;

    QMenu leftMenu;
    QMenu rightMenu;

    void leftButtonClicked();
    void rightButtonClicked();

private slots:
    void menuAddSplit();
    void menuCloseSplit();
    void menuMoveSplit();
    void menuChangeChannel();
    void menuClearChat();
    void menuOpenChannel();
    void menuPopupPlayer();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();
};

#endif  // CHATWIDGETHEADER_H
