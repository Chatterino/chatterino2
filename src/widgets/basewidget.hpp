#pragma once

#include <QWidget>
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace singletons {
class ThemeManager;
}  // namespace singletons

namespace widgets {

class BaseWindow;

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(singletons::ThemeManager &_themeManager, QWidget *parent,
                        Qt::WindowFlags f = Qt::WindowFlags());
    explicit BaseWidget(BaseWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~BaseWidget();

    singletons::ThemeManager &themeManager;

    float getScale() const;
    pajlada::Signals::Signal<float> scaleChanged;

    QSize getScaleIndependantSize() const;
    int getScaleIndependantWidth() const;
    int getScaleIndependantHeight() const;
    void setScaleIndependantSize(int width, int height);
    void setScaleIndependantSize(QSize);
    void setScaleIndependantWidth(int value);
    void setScaleIndependantHeight(int value);

protected:
    void childEvent(QChildEvent *) override;

    virtual void scaleChangedEvent(float newScale);
    virtual void themeRefreshEvent();

    void setScale(float value);

private:
    void init();
    float scale = 1.f;
    QSize scaleIndependantSize;

    std::vector<BaseWidget *> widgets;

    pajlada::Signals::Connection themeConnection;

    static void setScaleRecursive(float scale, QObject *object);

    friend class BaseWindow;
};

}  // namespace widgets
}  // namespace chatterino
