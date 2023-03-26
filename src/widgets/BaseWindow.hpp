#pragma once

#include "common/FlagsEnum.hpp"
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
enum class TitleBarButtonStyle;

class BaseWindow : public BaseWidget
{
    Q_OBJECT

public:
    enum Flags {
        None = 0,
        EnableCustomFrame = 1,
        Frameless = 2,
        TopMost = 4,
        DisableCustomScaling = 8,
        FramelessDraggable = 16,
        DontFocus = 32,
        Dialog = 64,
        DisableLayoutSave = 128,
    };

    enum ActionOnFocusLoss { Nothing, Delete, Close, Hide };

    explicit BaseWindow(FlagsEnum<Flags> flags_ = None,
                        QWidget *parent = nullptr);
    ~BaseWindow() override;

    void setInitialBounds(const QRect &bounds);
    QRect getBounds();

    QWidget *getLayoutContainer();
    bool hasCustomWindowFrame();
    TitleBarButton *addTitleBarButton(const TitleBarButtonStyle &style,
                                      std::function<void()> onClicked);
    EffectLabel *addTitleBarLabel(std::function<void()> onClicked);

    void setStayInScreenRect(bool value);
    bool getStayInScreenRect() const;

    void setActionOnFocusLoss(ActionOnFocusLoss value);
    ActionOnFocusLoss getActionOnFocusLoss() const;

    void moveTo(QWidget *widget, QPoint point, bool offset = true);

    virtual float scale() const override;
    float qtFontScale() const;

    pajlada::Signals::NoArgSignal closing;

    static bool supportsCustomWindowFrame();

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    bool nativeEvent(const QByteArray &eventType, void *message,
                     qintptr *result) override;
#else
    bool nativeEvent(const QByteArray &eventType, void *message,
                     long *result) override;
#endif
    virtual void scaleChangedEvent(float) override;

    virtual void paintEvent(QPaintEvent *) override;

    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void moveEvent(QMoveEvent *) override;
    virtual void closeEvent(QCloseEvent *) override;
    virtual void showEvent(QShowEvent *) override;

    virtual void themeChangedEvent() override;
    virtual bool event(QEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    QPointF movingRelativePos;
    bool moving{};

    void updateScale();

    boost::optional<QColor> overrideBackgroundColor_;

private:
    void init();
    void moveIntoDesktopRect(QPoint point);
    void calcButtonsSizes();
    void drawCustomWindowFrame(QPainter &painter);
    void onFocusLost();

    bool handleDPICHANGED(MSG *msg);
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
    bool stayInScreenRect_ = false;
    bool shown_ = false;
    FlagsEnum<Flags> flags_;
    float nativeScale_ = 1;
    bool isResizeFixing_ = false;

    struct {
        QLayout *windowLayout = nullptr;
        QHBoxLayout *titlebarBox = nullptr;
        QWidget *titleLabel = nullptr;
        TitleBarButton *minButton = nullptr;
        TitleBarButton *maxButton = nullptr;
        TitleBarButton *exitButton = nullptr;
        QWidget *layoutBase = nullptr;
        std::vector<Button *> buttons;
    } ui_;

#ifdef USEWINSDK
    QRect initalBounds_;
    QRect currentBounds_;
    QRect nextBounds_;
    QTimer useNextBounds_;
    bool isNotMinimizedOrMaximized_{};
#endif

    pajlada::Signals::SignalHolder connections_;

    friend class BaseWidget;
};

}  // namespace chatterino
