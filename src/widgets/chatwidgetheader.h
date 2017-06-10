#pragma once

#include "signallabel.h"
#include "widgets/chatwidgetheaderbutton.h"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QWidget>

namespace chatterino {
namespace widgets {
class ChatWidget;

class ChatWidgetHeader : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidgetHeader(ChatWidget *_chatWidget);

    // Update palette from global color scheme
    void updateColors();

    // Update channel text from chat widget
    void updateChannelText();

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    ChatWidget *chatWidget;

    QPoint _dragStart;
    bool _dragging;

    QHBoxLayout _hbox;

    ChatWidgetHeaderButton _leftLabel;
    SignalLabel _middleLabel;
    ChatWidgetHeaderButton _rightLabel;

    QMenu _leftMenu;
    QMenu _rightMenu;

    void leftButtonClicked();
    void rightButtonClicked();

private slots:
    void menuAddSplit();
    void menuCloseSplit();
    void menuMoveSplit();
    void menuPopup();
    void menuChangeChannel();
    void menuClearChat();
    void menuOpenChannel();
    void menuPopupPlayer();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();
};

}  // namespace widgets
}  // namespace chatterino
