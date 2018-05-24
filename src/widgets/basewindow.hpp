#pragma once

#include "basewidget.hpp"
#include "widgets/helper/titlebarbutton.hpp"

#include <functional>

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
    enum Flags { None = 0, EnableCustomFrame = 1, FrameLess = 2, TopMost = 4 };

    explicit BaseWindow(QWidget *parent = nullptr, Flags flags = None);

    QWidget *getLayoutContainer();
    bool hasCustomWindowFrame();
    void addTitleBarButton(const TitleBarButton::Style &style, std::function<void()> onClicked);
    RippleEffectLabel *addTitleBarLabel(std::function<void()> onClicked);

    void setStayInScreenRect(bool value);
    bool getStayInScreenRect() const;

    void moveTo(QWidget *widget, QPoint point);

    Flags getFlags();

protected:
#ifdef USEWINSDK
    void showEvent(QShowEvent *) override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    void paintEvent(QPaintEvent *) override;
    void scaleChangedEvent(float) override;
#endif

    void changeEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void resizeEvent(QResizeEvent *) override;

    void themeRefreshEvent() override;

private:
    void init();
    void moveIntoDesktopRect(QWidget *parent);
    void calcButtonsSizes();

    bool enableCustomFrame;
    bool frameless;
    bool stayInScreenRect = false;
    bool shown = false;
    Flags flags;

    struct {
        QHBoxLayout *titlebarBox = nullptr;
        QWidget *titleLabel = nullptr;
        TitleBarButton *minButton = nullptr;
        TitleBarButton *maxButton = nullptr;
        TitleBarButton *exitButton = nullptr;
        QWidget *layoutBase = nullptr;
        std::vector<RippleEffectButton *> buttons;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
