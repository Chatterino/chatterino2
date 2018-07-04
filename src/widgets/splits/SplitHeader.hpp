#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/helper/RippleEffectLabel.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QAction>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QWidget>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <vector>

namespace chatterino {

class Split;
class Label;

class SplitHeader : public BaseWidget, pajlada::Signals::SignalHolder
{
    Q_OBJECT

public:
    explicit SplitHeader(Split *_chatWidget);
    virtual ~SplitHeader() override;

    // Update channel text from chat widget
    void updateChannelText();
    void updateModerationModeIcon();
    void updateModes();

protected:
    virtual void scaleChangedEvent(float) override;
    virtual void themeRefreshEvent() override;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void mousePressEvent(QMouseEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;
    virtual void mouseMoveEvent(QMouseEvent *event) override;
    virtual void enterEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void rightButtonClicked();
    void initializeChannelSignals();
    void addModeActions(QMenu &menu);

    Split *const split;

    QPoint dragStart;
    bool dragging = false;
    bool doubleClicked = false;
    bool showingHelpTooltip = false;

    pajlada::Signals::Connection onlineStatusChangedConnection;

    RippleEffectButton *dropdownButton = nullptr;
    //    Label *titleLabel;
    Label *titleLabel = nullptr;
    Label *modeButton = nullptr;
    RippleEffectButton *moderationButton = nullptr;
    RippleEffectButton *moderationExtraButton = nullptr;

    QMenu dropdownMenu;
    QMenu modeMenu;

    pajlada::Signals::NoArgSignal modeUpdateRequested_;

    QString tooltip;
    bool isLive;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

public slots:
    void addDropdownItems(RippleEffectButton *label);
    void addModeItems(RippleEffectLabel *label);

    void menuMoveSplit();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();
};

}  // namespace chatterino
