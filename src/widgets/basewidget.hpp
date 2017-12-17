#pragma once

#include <QWidget>

//#include "windowmanager.hpp"

namespace chatterino {

class ColorScheme;

namespace widgets {

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    //    explicit BaseWidget(ColorScheme &_colorScheme, WindowManager &windowManager, QWidget
    //    *parent);
    explicit BaseWidget(ColorScheme &_colorScheme, QWidget *parent);

    explicit BaseWidget(BaseWidget *parent);

    explicit BaseWidget(QWidget *parent = nullptr);

    ColorScheme &colorScheme;

    float getDpiMultiplier();

    // protected:
    //    WindowManager &windowManager;

protected:
#ifdef USEWINSDK
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

    virtual void dpiMultiplyerChanged(float oldDpi, float newDpi)
    {
    }
    void initAsWindow();

private:
    float dpiMultiplier = 1.f;

    void init();

    virtual void refreshTheme();
};

}  // namespace widgets
}  // namespace chatterino
