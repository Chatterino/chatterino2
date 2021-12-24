#pragma once

#include "widgets/BaseWidget.hpp"

#include <QMenu>
#include <QPoint>
#include <memory>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <vector>

#include <QElapsedTimer>

namespace chatterino {

class Button;
class EffectLabel;
class Label;
class Split;

class SplitHeader final : public BaseWidget
{
    Q_OBJECT

public:
    explicit SplitHeader(Split *_chatWidget);

    void setAddButtonVisible(bool value);
    void setViewersButtonVisible(bool value);

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

    /**
     * @brief   Reset the thumbnail data and timer so a new
     *          thumbnail can be fetched
     **/
    void resetThumbnail();

    void handleChannelChanged();

    Split *const split_{};
    QString tooltipText_{};
    bool isLive_{false};
    QString thumbnail_;
    QElapsedTimer lastThumbnail_;

    // ui
    Button *dropdownButton_{};
    Label *titleLabel_{};
    EffectLabel *modeButton_{};
    Button *moderationButton_{};
    Button *viewersButton_{};
    Button *addButton_{};

    // states
    QPoint dragStart_{};
    bool dragging_{false};
    bool doubleClicked_{false};
    bool menuVisible_{false};

    // signals
    pajlada::Signals::NoArgSignal modeUpdateRequested_;
    pajlada::Signals::SignalHolder managedConnections_;
    pajlada::Signals::SignalHolder channelConnections_;

public slots:
    void reloadChannelEmotes();
    void reloadSubscriberEmotes();
    void reconnect();
};

}  // namespace chatterino
