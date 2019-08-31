#pragma once

#include "widgets/BaseWidget.hpp"

#include <functional>
#include <pajlada/signals/signalholder.hpp>

class QHBoxLayout;
struct tagMSG;
typedef struct tagMSG MSG;

namespace AB_NAMESPACE {

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
    };

    enum ActionOnFocusLoss { Nothing, Delete, Close, Hide };

    explicit BaseWindow(QWidget *parent = nullptr, Flags flags_ = None);

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

    Flags getFlags();

    pajlada::Signals::NoArgSignal closing;

    static bool supportsCustomWindowFrame();

protected:
    virtual bool nativeEvent(const QByteArray &eventType, void *message,
                             long *result) override;
    virtual void scaleChangedEvent(float) override;

    virtual void paintEvent(QPaintEvent *) override;

    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void moveEvent(QMoveEvent *) override;
    virtual void closeEvent(QCloseEvent *) override;

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
    void moveIntoDesktopRect(QWidget *parent);
    void calcButtonsSizes();
    void drawCustomWindowFrame(QPainter &painter);
    void onFocusLost();

    bool handleDPICHANGED(MSG *msg);
    bool handleSHOWWINDOW(MSG *msg);
    bool handleNCCALCSIZE(MSG *msg, long *result);
    bool handleSIZE(MSG *msg);
    bool handleNCHITTEST(MSG *msg, long *result);

    bool enableCustomFrame_;
    ActionOnFocusLoss actionOnFocusLoss_ = Nothing;
    bool frameless_;
    bool stayInScreenRect_ = false;
    bool shown_ = false;
    Flags flags_;
    float nativeScale_ = 1;

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

    pajlada::Signals::SignalHolder connections_;
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;

    friend class BaseWidget;
};

}  // namespace AB_NAMESPACE
