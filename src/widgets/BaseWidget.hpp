// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QShortcut>
#include <QWidget>

#include <optional>

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

    std::optional<float> overrideScale() const;
    void setOverrideScale(std::optional<float>);

    QSize scaleIndependentSize() const;
    int scaleIndependentWidth() const;
    int scaleIndependentHeight() const;
    void setScaleIndependentSize(int width, int height);
    void setScaleIndependentSize(QSize);
    void setScaleIndependentWidth(int value);
    void setScaleIndependentHeight(int value);

protected:
    void childEvent(QChildEvent *) override;
    void showEvent(QShowEvent *) override;

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
    std::optional<float> overrideScale_;
    QSize scaleIndependentSize_;

    std::vector<BaseWidget *> widgets_;

    friend class BaseWindow;
    friend class OverlayWindow;  // for setScale()
};

}  // namespace chatterino
