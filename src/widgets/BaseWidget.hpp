#pragma once

#include <QWidget>
#include <boost/optional.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class Themes;
class BaseWindow;

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(QWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~BaseWidget() override;

    virtual float getScale() const;
    pajlada::Signals::Signal<float> scaleChanged;

    void setOverrideScale(boost::optional<float>);
    boost::optional<float> getOverrideScale() const;

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

    virtual void showEvent(QShowEvent *) override;

    void setScale(float value);

    Themes *themeManager;

private:
    void init();
    float scale = 1.f;
    boost::optional<float> overrideScale = boost::none;
    QSize scaleIndependantSize;

    std::vector<BaseWidget *> widgets;

    pajlada::Signals::Connection themeConnection;

    friend class BaseWindow;
};

}  // namespace chatterino
