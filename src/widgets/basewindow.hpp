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
    explicit BaseWindow(singletons::ThemeManager &_themeManager, QWidget *parent,
                        bool enableCustomFrame = false);
    explicit BaseWindow(BaseWidget *parent, bool enableCustomFrame = false);
    explicit BaseWindow(QWidget *parent = nullptr, bool enableCustomFrame = false);

    QWidget *getLayoutContainer();
    bool hasCustomWindowFrame();
    void addTitleBarButton(const TitleBarButton::Style &style, std::function<void()> onClicked);
    RippleEffectLabel *addTitleBarLabel(std::function<void()> onClicked);

    void setStayInScreenRect(bool value);
    bool getStayInScreenRect() const;

    void moveTo(QWidget *widget, QPoint point);

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
    bool stayInScreenRect = false;
    bool shown = false;

    struct {
        QHBoxLayout *titlebarBox;
        QWidget *titleLabel;
        TitleBarButton *minButton = nullptr;
        TitleBarButton *maxButton = nullptr;
        TitleBarButton *exitButton = nullptr;
        QWidget *layoutBase;
        std::vector<RippleEffectButton *> buttons;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
