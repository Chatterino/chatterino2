#pragma once

#include "basewidget.hpp"

namespace chatterino {
namespace widgets {

class BaseWindow : public BaseWidget
{
public:
    explicit BaseWindow(singletons::ThemeManager &_themeManager, QWidget *parent);
    explicit BaseWindow(BaseWidget *parent);
    explicit BaseWindow(QWidget *parent = nullptr);

protected:
#ifdef USEWINSDK
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

private:
    void init();
};
}  // namespace widgets
}  // namespace chatterino
