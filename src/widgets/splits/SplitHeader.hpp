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
    void updateRoomModes();

protected:
    virtual void scaleChangedEvent(float) override;
    virtual void themeChangedEvent() override;

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
    void setupModeLabel(RippleEffectLabel &label);
    std::unique_ptr<QMenu> createMainMenu();
    std::unique_ptr<QMenu> createChatModeMenu();

    Split *split_;

    QPoint dragStart_;
    bool dragging_ = false;
    bool doubleClicked_ = false;
    bool showingHelpTooltip_ = false;

    pajlada::Signals::Connection onlineStatusChangedConnection_;

    RippleEffectButton *dropdownButton_{};
    //    Label *titleLabel{};
    Label *titleLabel{};
    RippleEffectLabel *modeButton_{};
    RippleEffectButton *moderationButton_{};

    bool menuVisible_{};

    pajlada::Signals::NoArgSignal modeUpdateRequested_;

    QString tooltip_;
    bool isLive_;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;

public slots:
    void menuMoveSplit();
    void menuReloadChannelEmotes();
    void menuManualReconnect();
    void menuShowChangelog();
};

}  // namespace chatterino
