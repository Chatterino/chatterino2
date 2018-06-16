#pragma once

#include "basewidget.hpp"
#include "widgets/helper/titlebarbutton.hpp"

#include <functional>
#include <pajlada/signals/signalholder.hpp>

class QHBoxLayout;

namespace chatterino {
namespace widgets {

class RippleEffectButton;
class RippleEffectLabel;
class TitleBarButton;

class BaseWindow : public BaseWidget
{
    Q_OBJECT

public:
    enum Flags {
        None = 0,
        EnableCustomFrame = 1,
        Frameless = 2,
        TopMost = 4,
        DeleteOnFocusOut = 8,
        DisableCustomScaling = 16,
    };

    explicit BaseWindow(QWidget *parent = nullptr, Flags flags_ = None);

    QWidget *getLayoutContainer();
    bool hasCustomWindowFrame();
    void addTitleBarButton(const TitleBarButton::Style &style, std::function<void()> onClicked);
    RippleEffectLabel *addTitleBarLabel(std::function<void()> onClicked);

    void setStayInScreenRect(bool value);
    bool getStayInScreenRect() const;

    void moveTo(QWidget *widget, QPoint point, bool offset = true);

    Flags getFlags();

protected:
#ifdef USEWINSDK
    virtual void showEvent(QShowEvent *) override;
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    virtual void scaleChangedEvent(float) override;
#endif

    virtual void paintEvent(QPaintEvent *) override;

    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void themeRefreshEvent() override;
    virtual bool event(QEvent *event) override;
    virtual void wheelEvent(QWheelEvent *event) override;

    void updateScale();

private:
    void init();
    void moveIntoDesktopRect(QWidget *parent);
    void calcButtonsSizes();

    bool enableCustomFrame_;
    bool frameless_;
    bool stayInScreenRect_ = false;
    bool shown_ = false;
    Flags flags_;
    float nativeScale_ = 1;

    struct {
        QHBoxLayout *titlebarBox = nullptr;
        QWidget *titleLabel = nullptr;
        TitleBarButton *minButton = nullptr;
        TitleBarButton *maxButton = nullptr;
        TitleBarButton *exitButton = nullptr;
        QWidget *layoutBase = nullptr;
        std::vector<RippleEffectButton *> buttons;
    } ui_;

    pajlada::Signals::SignalHolder connections_;
};

}  // namespace widgets
}  // namespace chatterino
