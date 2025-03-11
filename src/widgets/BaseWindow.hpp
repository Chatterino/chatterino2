#pragma once

#include "common/FlagsEnum.hpp"
#include "util/WidgetHelpers.hpp"
#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QTimer>

#include <functional>

class QHBoxLayout;
struct tagMSG;
typedef struct tagMSG MSG;

namespace chatterino {

class Button;
class EffectLabel;
class TitleBarButton;
class TitleBarButtons;
enum class TitleBarButtonStyle;

class BaseWindow : public BaseWidget
{
    Q_OBJECT

public:
    enum Flags {
        None = 0,
        EnableCustomFrame = 1 << 0,
        Frameless = 1 << 1,
        TopMost = 1 << 2,
        DisableCustomScaling = 1 << 3,
        FramelessDraggable = 1 << 4,
        DontFocus = 1 << 5,
        Dialog = 1 << 6,
        DisableLayoutSave = 1 << 7,
        BoundsCheckOnShow = 1 << 8,
        ClearBuffersOnDpiChange = 1 << 9,
    };

    enum ActionOnFocusLoss { Nothing, Delete, Close, Hide };

    explicit BaseWindow(FlagsEnum<Flags> flags_ = None,
                        QWidget *parent = nullptr);
    ~BaseWindow() override;

    void setInitialBounds(QRect bounds, widgets::BoundsChecking mode);
    QRect getBounds() const;

    QWidget *getLayoutContainer();
    bool hasCustomWindowFrame() const;
    TitleBarButton *addTitleBarButton(const TitleBarButtonStyle &style,
                                      std::function<void()> onClicked);
    EffectLabel *addTitleBarLabel(std::function<void()> onClicked);

    void setActionOnFocusLoss(ActionOnFocusLoss value);
    ActionOnFocusLoss getActionOnFocusLoss() const;

    void moveTo(QPoint point, widgets::BoundsChecking mode);

    /**
     * Moves the window to the given point and does bounds checking according to `mode`
     * Depending on the platform, either the move or the show will take place first
     **/
    void showAndMoveTo(QPoint point, widgets::BoundsChecking mode);

    /// @brief Applies the last moveTo operation if that one was bounds-checked
    ///
    /// If there was a previous moveTo or showAndMoveTo operation with a mode
    /// other than `Off`, a moveTo is repeated with the last supplied @a point
    /// and @a mode. Note that in the case of showAndMoveTo, moveTo is run.
    ///
    /// @returns true if there was a previous bounds-checked moveTo operation
    bool applyLastBoundsCheck();

    float scale() const override;

    /// @returns true if the window is the top-most window.
    ///          Either #setTopMost was called or the `TopMost` flag is set which overrides this
    bool isTopMost() const;
    /// Updates the window's top-most status
    /// If the `TopMost` flag is set, this is a no-op
    void setTopMost(bool topMost);

    pajlada::Signals::NoArgSignal closing;
    pajlada::Signals::NoArgSignal leaving;

    static bool supportsCustomWindowFrame();

Q_SIGNALS:
    void topMostChanged(bool topMost);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message,
                     qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;
#endif
    void scaleChangedEvent(float) override;

    void paintEvent(QPaintEvent *) override;

    void changeEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void closeEvent(QCloseEvent *) override;
    void showEvent(QShowEvent *) override;

    void themeChangedEvent() override;
    bool event(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    QPointF movingRelativePos;
    bool moving{};

    /// @returns The scale this window wants to be at.
    virtual float desiredScale() const;
    void updateScale();

    std::optional<QColor> overrideBackgroundColor_;

private:
    void init();

    void calcButtonsSizes();
    void drawCustomWindowFrame(QPainter &painter);
    void onFocusLost();

    static void applyScaleRecursive(QObject *root, float scale);

    bool handleSHOWWINDOW(MSG *msg);
    bool handleSIZE(MSG *msg);
    bool handleMOVE(MSG *msg);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool handleNCCALCSIZE(MSG *msg, qintptr *result);
    bool handleNCHITTEST(MSG *msg, qintptr *result);
#else
    bool handleNCCALCSIZE(MSG *msg, long *result);
    bool handleNCHITTEST(MSG *msg, long *result);
#endif

    bool enableCustomFrame_;
    ActionOnFocusLoss actionOnFocusLoss_ = Nothing;
    bool frameless_;
    bool shown_ = false;
    FlagsEnum<Flags> flags_;
    bool isTopMost_ = false;

    struct {
        QLayout *windowLayout = nullptr;
        QHBoxLayout *titlebarBox = nullptr;
        QWidget *titleLabel = nullptr;
        TitleBarButtons *titlebarButtons = nullptr;
        QWidget *layoutBase = nullptr;
        std::vector<Button *> buttons;
    } ui_;

    /// The last @a pos from moveTo and showAndMoveTo
    QPoint lastBoundsCheckPosition_;
    /// The last @a mode from moveTo and showAndMoveTo
    widgets::BoundsChecking lastBoundsCheckMode_ = widgets::BoundsChecking::Off;

#ifdef USEWINSDK
    void updateRealSize();
    /// @brief Returns the HWND of this window if it has one
    ///
    /// A QWidget only has an HWND if it has been created. Before that,
    /// accessing `winID()` will create the window which can lead to unintended
    /// bugs.
    std::optional<HWND> safeHWND() const;

    /// @brief Tries to apply the `isTopMost_` setting
    ///
    /// If the setting couldn't be applied (because the window wasn't created
    /// yet), the operation is repeated after a short delay.
    ///
    /// @pre When calling from outside this method, `waitingForTopMost_` must
    ///      be `false` to avoid too many pending calls.
    /// @post If an operation was queued to be executed after some delay,
    ///       `waitingForTopMost_` will be set to `true`.
    void tryApplyTopMost();
    bool waitingForTopMost_ = false;

    QRect initalBounds_;
    QRect currentBounds_;
    QTimer useNextBounds_;
    bool isNotMinimizedOrMaximized_{};
    bool lastEventWasNcMouseMove_ = false;
    /// The real bounds of the window as returned by
    /// GetWindowRect. Used for drawing.
    QRect realBounds_;
    bool isMaximized_ = false;
#endif

    pajlada::Signals::SignalHolder connections_;

    friend class BaseWidget;
};

}  // namespace chatterino
