#pragma once

#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(ColorScheme &_colorScheme, QWidget *parent);

    explicit BaseWidget(BaseWidget *parent);

    explicit BaseWidget(QWidget *parent = nullptr);

    ColorScheme &colorScheme;

    float getDpiMultiplier();

protected:
#ifdef USEWINSDK
    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;
#endif

    virtual void focusOutEvent(QFocusEvent *) override;

    // XXX: Should this be pure virtual?
    // fourtf: hell no
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
