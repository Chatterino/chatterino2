#pragma once

#include <QWidget>
#include <boost/optional.hpp>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class Theme;
class BaseWindow;

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(QWidget *parent, Qt::WindowFlags f = Qt::WindowFlags());

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
    virtual void childEvent(QChildEvent *) override;
    virtual void showEvent(QShowEvent *) override;

    virtual void scaleChangedEvent(float newScale);
    virtual void themeChangedEvent();

    void setScale(float value);

    Theme *theme;

private:
    float scale_ = 1.f;
    boost::optional<float> overrideScale_ = boost::none;
    QSize scaleIndependantSize_;

    std::vector<BaseWidget *> widgets_;

    pajlada::Signals::SignalHolder signalHolder_;

    friend class BaseWindow;
};

}  // namespace chatterino
