#pragma once

#include <QWidget>

namespace chatterino {
namespace singletons {
class ThemeManager;
}

namespace widgets {

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(singletons::ThemeManager &_themeManager, QWidget *parent);

    explicit BaseWidget(BaseWidget *parent);

    explicit BaseWidget(QWidget *parent = nullptr);

    singletons::ThemeManager &themeManager;

    float getDpiMultiplier();

protected:
#ifdef USEWINSDK
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;

    virtual void dpiMultiplierChanged(float /*oldDpi*/, float /*newDpi*/)
    {
    }
    void initAsWindow();

private:
    bool isWindow = false;
    float dpiMultiplier = 1.f;

    void init();

    virtual void refreshTheme();
};

}  // namespace widgets
}  // namespace chatterino
