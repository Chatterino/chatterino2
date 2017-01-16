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
    ChatWidgetHeader(ChatWidget *parent);

    ChatWidget *
    chatWidget()
    {
        return m_chatWidget;
    }

    void updateColors();
    void updateChannelText();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);

private:
    ChatWidget *m_chatWidget;

    QPoint m_dragStart;
    bool m_dragging;

    QHBoxLayout m_hbox;

    ChatWidgetHeaderButton m_leftLabel;
    QLabel m_middleLabel;
    ChatWidgetHeaderButton m_rightLabel;

    QMenu m_leftMenu;
    QMenu m_rightMenu;

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
