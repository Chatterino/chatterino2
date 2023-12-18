#pragma once

#include "common/Channel.hpp"
#include "controllers/hotkeys/GlobalShortcutFwd.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/TitlebarButton.hpp"

#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QPropertyAnimation>
#include <QWidget>

class QGraphicsDropShadowEffect;

namespace chatterino {

class OverlayWindow : public QWidget
{
    Q_OBJECT
public:
    OverlayWindow(IndirectChannel channel);
    ~OverlayWindow() override;
    OverlayWindow(const OverlayWindow &) = delete;
    OverlayWindow(OverlayWindow &&) = delete;
    OverlayWindow &operator=(const OverlayWindow &) = delete;
    OverlayWindow &operator=(OverlayWindow &&) = delete;

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Q_PROPERTY(double interactionProgress READ interactionProgress WRITE
                   setInteractionProgress)

    double interactionProgress() const;
    void setInteractionProgress(double progress);

    void startInteraction();
    void endInteraction();

    void setOverrideCursor(const QCursor &cursor);

    IndirectChannel channel_;
    pajlada::Signals::SignalHolder holder_;
    ChannelView channelView_;
    QGraphicsDropShadowEffect *dropShadow_;

    bool interacting_ = false;
    bool moving_ = false;
    QPoint moveOrigin_;

    double interactionProgress_ = 0.0;
    QPropertyAnimation interactAnimation_;

    TitleBarButton closeButton_;

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    std::unique_ptr<GlobalShortcut> shortcut_;
    bool inert_ = false;
#endif
};

}  // namespace chatterino
