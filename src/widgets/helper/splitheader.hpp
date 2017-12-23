#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/helper/signallabel.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QWidget>
#include <boost/signals2/connection.hpp>

namespace chatterino {

class ColorScheme;

namespace widgets {

class Split;

class SplitHeader : public BaseWidget
{
    Q_OBJECT

public:
    explicit SplitHeader(Split *_chatWidget);
    // Update channel text from chat widget
    void updateChannelText();

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    Split *const chatWidget;

    QPoint dragStart;
    bool dragging = false;

    boost::signals2::connection onlineStatusChangedConnection;

    QHBoxLayout hbox;

    // top left
    RippleEffectLabel leftLabel;
    QMenu leftMenu;

    // center
    SignalLabel channelNameLabel;

    // top right
    RippleEffectLabel rightLabel;
    QMenu rightMenu;

    void leftButtonClicked();
    void rightButtonClicked();

    virtual void refreshTheme() override;

    void initializeChannelSignals();

    QString tooltip;
    bool isLive;

public slots:
    void menuMoveSplit();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();
};

}  // namespace widgets
}  // namespace chatterino
