#include "widgets/BaseWidget.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "singletons/Theme.hpp"
#include "widgets/BaseWindow.hpp"

#include <QChildEvent>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QtGlobal>

#include <algorithm>

namespace chatterino {

BaseWidget::BaseWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , theme(getApp()->getThemes())
{
    auto *baseWidget = dynamic_cast<BaseWidget *>(this->window());
    if (baseWidget && baseWidget != this)
    {
        this->scale_ = baseWidget->scale_;
    }

    this->signalHolder_.managedConnect(this->theme->updated, [this]() {
        this->themeChangedEvent();

        this->update();
    });
}
void BaseWidget::clearShortcuts()
{
    for (auto *shortcut : this->shortcuts_)
    {
        shortcut->setKey(QKeySequence());
        shortcut->removeEventFilter(this);
        shortcut->deleteLater();
    }
    this->shortcuts_.clear();
}

float BaseWidget::scale() const
{
    if (this->overrideScale_)
    {
        return *this->overrideScale_;
    }

    if (auto *baseWidget = dynamic_cast<BaseWidget *>(this->window()))
    {
        return baseWidget->scale_;
    }

    return 1.F;
}

void BaseWidget::setScale(float value)
{
    if (this->scale_ == value)
    {
        return;
    }

    this->scale_ = value;

    this->scaleChangedEvent(this->scale());
    this->scaleChanged.invoke(this->scale());

    this->setScaleIndependantSize(this->scaleIndependantSize());
}

void BaseWidget::setOverrideScale(std::optional<float> value)
{
    this->overrideScale_ = value;
    this->setScale(this->scale());
}

std::optional<float> BaseWidget::overrideScale() const
{
    return this->overrideScale_;
}

QSize BaseWidget::scaleIndependantSize() const
{
    return this->scaleIndependantSize_;
}

int BaseWidget::scaleIndependantWidth() const
{
    return this->scaleIndependantSize_.width();
}

int BaseWidget::scaleIndependantHeight() const
{
    return this->scaleIndependantSize_.height();
}

void BaseWidget::setScaleIndependantSize(int width, int height)
{
    this->setScaleIndependantSize(QSize(width, height));
}

void BaseWidget::setScaleIndependantSize(QSize size)
{
    this->scaleIndependantSize_ = size;

    if (size.width() > 0)
    {
        this->setFixedWidth(int(size.width() * this->scale()));
    }
    if (size.height() > 0)
    {
        this->setFixedHeight(int(size.height() * this->scale()));
    }
}

void BaseWidget::setScaleIndependantWidth(int value)
{
    this->setScaleIndependantSize(
        QSize(value, this->scaleIndependantSize_.height()));
}

void BaseWidget::setScaleIndependantHeight(int value)
{
    this->setScaleIndependantSize(
        QSize(this->scaleIndependantSize_.width(), value));
}

void BaseWidget::childEvent(QChildEvent *event)
{
    if (event->added())
    {
        // add element if it's a basewidget
        if (auto *widget = dynamic_cast<BaseWidget *>(event->child()))
        {
            this->widgets_.push_back(widget);
        }
    }
    else if (event->removed())
    {
        // find element to be removed
        auto it = std::find_if(this->widgets_.begin(), this->widgets_.end(),
                               [&](auto &&x) {
                                   return x == event->child();
                               });

        // remove if found
        if (it != this->widgets_.end())
        {
            this->widgets_.erase(it);
        }
    }
}

void BaseWidget::showEvent(QShowEvent *)
{
    this->setScale(this->scale());
    this->themeChangedEvent();
}

void BaseWidget::scaleChangedEvent(float newDpi)
{
}

void BaseWidget::themeChangedEvent()
{
    // Do any color scheme updates here
}

}  // namespace chatterino
