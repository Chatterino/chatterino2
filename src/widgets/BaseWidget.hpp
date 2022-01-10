#pragma once

#include <QShortcut>
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
    explicit BaseWidget(QWidget *parent = nullptr,
                        Qt::WindowFlags f = Qt::WindowFlags());

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
    [[deprecated("addShortcuts called without overriding it")]] virtual void
        addShortcuts()
    {
    }

    void setScale(float value);

    Theme *theme;

    std::vector<QShortcut *> shortcuts_;
    void clearShortcuts();
    pajlada::Signals::SignalHolder signalHolder_;

private:
    float scale_{1.f};
    boost::optional<float> overrideScale_;
    QSize scaleIndependantSize_;

    std::vector<BaseWidget *> widgets_;

    friend class BaseWindow;
};

}  // namespace chatterino
