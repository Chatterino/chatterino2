#pragma once

#include "widgets/BaseWidget.hpp"

#include <QMenu>
#include <QPoint>
#include <memory>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <vector>

namespace chatterino {

class Button;
class EffectLabel;
class Label;
class Split;

class SplitHeader final : public BaseWidget, pajlada::Signals::SignalHolder
{
    Q_OBJECT

public:
    explicit SplitHeader(Split *_chatWidget);

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
    void initializeLayout();
    void initializeModeSignals(EffectLabel &label);
    std::unique_ptr<QMenu> createMainMenu();
    std::unique_ptr<QMenu> createChatModeMenu();
    void handleChannelChanged();

    Split *const split_{};
    QString tooltipText_{};
    bool isLive_{false};

    // ui
    Button *dropdownButton_{};
    Label *titleLabel{};
    EffectLabel *modeButton_{};
    Button *moderationButton_{};

    // states
    QPoint dragStart_{};
    bool dragging_{false};
    bool doubleClicked_{false};
    bool showingHelpTooltip_{false};
    bool menuVisible_{false};

    // signals
    pajlada::Signals::NoArgSignal modeUpdateRequested_;
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
    std::vector<pajlada::Signals::ScopedConnection> channelConnections_;

public slots:
    void moveSplit();
    void reloadChannelEmotes();
    void reconnect();
};

}  // namespace chatterino
