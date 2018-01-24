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
    explicit BaseWidget(singletons::ThemeManager &_themeManager, QWidget *parent,
                        Qt::WindowFlags f = Qt::WindowFlags());
    explicit BaseWidget(BaseWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());
    explicit BaseWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    singletons::ThemeManager &themeManager;

    float getDpiMultiplier();

protected:
    virtual void dpiMultiplierChanged(float /*oldDpi*/, float /*newDpi*/)
    {
    }

    float dpiMultiplier = 1.f;

    virtual void refreshTheme();

private:
    void init();
};

}  // namespace widgets
}  // namespace chatterino
