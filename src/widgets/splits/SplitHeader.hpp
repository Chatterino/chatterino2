#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/TooltipWidget.hpp"

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

    void updateChannelText();
    void updateIcons();
    // Invoked when SplitHeader should update anything refering to a TwitchChannel's mode
    // has changed (e.g. sub mode toggled)
    void updateRoomModes();

protected:
    void scaleChangedEvent(float scale) override;
    void themeChangedEvent() override;

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
#endif
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
    TooltipWidget *const tooltipWidget_{};
    bool isLive_{false};
    QString thumbnail_;
    QElapsedTimer lastThumbnail_;
    std::chrono::steady_clock::time_point lastReloadedChannelEmotes_;
    std::chrono::steady_clock::time_point lastReloadedSubEmotes_;

    // ui
    Button *dropdownButton_{};
    Label *titleLabel_{};

    EffectLabel *modeButton_{};
    QAction *modeActionSetEmote{};
    QAction *modeActionSetSub{};
    QAction *modeActionSetSlow{};
    QAction *modeActionSetR9k{};
    QAction *modeActionSetFollowers{};

    Button *moderationButton_{};
    Button *chattersButton_{};
    Button *addButton_{};

    // states
    QPoint dragStart_{};
    bool dragging_{false};
    bool doubleClicked_{false};
    bool menuVisible_{false};

    // managedConnections_ contains connections for signals that are not managed by us
    // and don't change when the parent Split changes its underlying channel
    pajlada::Signals::SignalHolder managedConnections_;
    pajlada::Signals::SignalHolder channelConnections_;
    std::vector<boost::signals2::scoped_connection> bSignals_;

public Q_SLOTS:
    void reloadChannelEmotes();
    void reloadSubscriberEmotes();
    void reconnect();
};

}  // namespace chatterino
