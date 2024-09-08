#pragma once

#include "common/Channel.hpp"
#include "controllers/hotkeys/GlobalShortcutFwd.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/OverlayInteraction.hpp"

#include <pajlada/signals/scoped-connection.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QTimer>
#include <QWidget>

#ifdef Q_OS_WIN
#    include <QtGui/qwindowdefs_win.h>
#endif

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

    void setOverrideCursor(const QCursor &cursor);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    using NativeResult = qintptr;
    using EnterEvent = QEnterEvent;
#else
    using NativeResult = long;
    using EnterEvent = QEvent;
#endif

    bool eventFilter(QObject *object, QEvent *event) override;
    void enterEvent(EnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

#ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray &eventType, void *message,
                     NativeResult *result) override;
#endif

private:
    void triggerFirstActivation();

    void startInteraction();
    void startShortInteraction();
    void endInteraction();

    void applyTheme();

#ifdef Q_OS_WIN
    void handleNCHITTEST(MSG *msg, qintptr *result);

    HCURSOR sizeAllCursor_;
#endif

    IndirectChannel channel_;
    pajlada::Signals::SignalHolder holder_;

    ChannelView channelView_;
    QGraphicsDropShadowEffect *dropShadow_;

    bool moving_ = false;
    QPoint moveOrigin_;

    OverlayInteraction interaction_;
    QTimer shortInteraction_;

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    void setInert(bool inert);

    std::unique_ptr<GlobalShortcut> shortcut_;
    bool inert_ = false;
#endif
};

}  // namespace chatterino
