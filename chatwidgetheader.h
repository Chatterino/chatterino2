#ifndef CHATWIDGETHEADER_H
#define CHATWIDGETHEADER_H

#include "chatwidgetheaderbutton.h"

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
    ChatWidgetHeader();
    ChatWidget *getChatWidget();
    void updateColors();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QPoint m_dragStart;
    bool m_dragging;

    QHBoxLayout hbox;

    ChatWidgetHeaderButton leftLabel;
    QLabel middleLabel;
    ChatWidgetHeaderButton rightLabel;

    QMenu leftMenu;
    QMenu rightMenu;

    void leftButtonClicked();
    void rightButtonClicked();

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
