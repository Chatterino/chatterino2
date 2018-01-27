#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/label.hpp"
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
#include <pajlada/settings/setting.hpp>

namespace chatterino {

namespace widgets {

class Split;

class SplitHeader : public BaseWidget
{
    Q_OBJECT

public:
    explicit SplitHeader(Split *_chatWidget);
    ~SplitHeader();

    // Update channel text from chat widget
    void updateChannelText();
    void updateModerationModeIcon();

protected:
    virtual void scaleChangedEvent(float) override;
    virtual void themeRefreshEvent() override;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    Split *const split;

    QPoint dragStart;
    bool dragging = false;

    boost::signals2::connection onlineStatusChangedConnection;

    RippleEffectButton *dropdownButton;
    //    Label *titleLabel;
    SignalLabel *titleLabel;
    RippleEffectButton *moderationButton;

    QMenu dropdownMenu;

    void rightButtonClicked();

    void initializeChannelSignals();

    QString tooltip;
    bool isLive;

public slots:
    void addDropdownItems(RippleEffectButton *label);

    void menuMoveSplit();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();
};

}  // namespace widgets
}  // namespace chatterino
