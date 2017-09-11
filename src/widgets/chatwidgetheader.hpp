#pragma once

#include "signallabel.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/chatwidgetheaderbutton.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class ChatWidget;

class ChatWidgetHeader : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetHeader(ChatWidget *_chatWidget);
    // Update channel text from chat widget
    void updateChannelText();
    void checkLive();

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    ChatWidget *const chatWidget;

    QPoint dragStart;
    bool dragging = false;

    QHBoxLayout hbox;

    // top left
    ChatWidgetHeaderButton leftLabel;
    QMenu leftMenu;

    // center
    SignalLabel channelNameLabel;

    // top right
    ChatWidgetHeaderButton rightLabel;
    QMenu rightMenu;

    void leftButtonClicked();
    void rightButtonClicked();

    virtual void refreshTheme() override;

public slots:
    void menuMoveSplit();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();

};

}  // namespace widgets
}  // namespace chatterino
