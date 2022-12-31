#pragma once

#include "widgets/BaseWidget.hpp"

#include <boost/signals2.hpp>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/connection.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QElapsedTimer>
#include <QMenu>
#include <QPoint>

#include <memory>
#include <vector>

namespace chatterino {

class Button;
class EffectLabel;
class Label;
class Split;

class SplitHeader final : public BaseWidget
{
    Q_OBJECT

public:
    explicit SplitHeader(Split *split);

    void setAddButtonVisible(bool value);
    void setViewersButtonVisible(bool value);

    void updateChannelText();
    void updateModerationModeIcon();
    void updateRoomModes();

protected:
    void scaleChangedEvent(float scale) override;
    void themeChangedEvent() override;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

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
    std::chrono::steady_clock::time_point lastReloadedChannelEmotes_;
    std::chrono::steady_clock::time_point lastReloadedSubEmotes_;

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
    std::vector<boost::signals2::scoped_connection> bSignals_;

public slots:
    void reloadChannelEmotes();
    void reloadSubscriberEmotes();
    void reconnect();
};

}  // namespace chatterino
