#pragma once

#include "basewidget.hpp"

class QHBoxLayout;

namespace chatterino {
namespace widgets {

class BaseWindow : public BaseWidget
{
public:
    explicit BaseWindow(singletons::ThemeManager &_themeManager, QWidget *parent,
                        bool enableCustomFrame = false);
    explicit BaseWindow(BaseWidget *parent, bool enableCustomFrame = false);
    explicit BaseWindow(QWidget *parent = nullptr, bool enableCustomFrame = false);

    QWidget *getLayoutContainer();
    bool hasCustomWindowFrame();
    void addTitleBarButton(const QString &text);

protected:
#ifdef USEWINSDK
    virtual void showEvent(QShowEvent *);
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
    virtual void paintEvent(QPaintEvent *event) override;
#endif

    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

    virtual void refreshTheme() override;

private:
    void init();

    bool enableCustomFrame;

    QHBoxLayout *titlebarBox;
    QWidget *titleLabel;
    QWidget *minButton;
    QWidget *maxButton;
    QWidget *exitButton;
    QWidget *layoutBase;
    std::vector<QWidget *> widgets;
};
}  // namespace widgets
}  // namespace chatterino
