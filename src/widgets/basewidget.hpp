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
    virtual void dpiMultiplierChanged(float /*oldDpi*/, float /*newDpi*/)
    {
    }

    float dpiMultiplier = 1.f;

private:
    void init();

    virtual void refreshTheme();
};

}  // namespace widgets
}  // namespace chatterino
