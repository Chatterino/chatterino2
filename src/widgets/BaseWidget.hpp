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

    virtual float scale() const;
    pajlada::Signals::Signal<float> scaleChanged;

    boost::optional<float> overrideScale() const;
    void setOverrideScale(boost::optional<float>);

    QSize scaleIndependantSize() const;
    int scaleIndependantWidth() const;
    int scaleIndependantHeight() const;
    void setScaleIndependantSize(int width, int height);
    void setScaleIndependantSize(QSize);
    void setScaleIndependantWidth(int value);
    void setScaleIndependantHeight(int value);

    float qtFontScale() const;

protected:
    virtual void childEvent(QChildEvent *) override;
    virtual void showEvent(QShowEvent *) override;

    virtual void scaleChangedEvent(float newScale);
    virtual void themeChangedEvent();

    void setScale(float value);

    Theme *theme;

private:
    float scale_{1.f};
    boost::optional<float> overrideScale_;
    QSize scaleIndependantSize_;

    std::vector<BaseWidget *> widgets_;

    pajlada::Signals::SignalHolder signalHolder_;

    friend class BaseWindow;
};

}  // namespace chatterino
