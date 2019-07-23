#include "widgets/BaseWidget.hpp"

#include "BaseSettings.hpp"
#include "BaseTheme.hpp"
#include "debug/Log.hpp"
#include "widgets/BaseWindow.hpp"

#include <QChildEvent>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QtGlobal>

namespace AB_NAMESPACE {

BaseWidget::BaseWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    // REMOVED
    this->theme = getTheme();

    this->signalHolder_.managedConnect(this->theme->updated, [this]() {
        this->themeChangedEvent();

        this->update();
    });
}

float BaseWidget::scale() const
{
    if (this->overrideScale_)
    {
        return this->overrideScale_.get();
    }
    else if (auto baseWidget = dynamic_cast<BaseWidget *>(this->window()))
    {
        return baseWidget->scale_;
    }
    else
    {
        return 1.f;
    }
}

void BaseWidget::setScale(float value)
{
    // update scale value
    this->scale_ = value;

    this->scaleChangedEvent(this->scale());
    this->scaleChanged.invoke(this->scale());

    this->setScaleIndependantSize(this->scaleIndependantSize());
}

void BaseWidget::setOverrideScale(boost::optional<float> value)
{
    this->overrideScale_ = value;
    this->setScale(this->scale());
}

boost::optional<float> BaseWidget::overrideScale() const
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

float BaseWidget::qtFontScale() const
{
    if (auto window = dynamic_cast<BaseWindow *>(this->window()))
    {
        return this->scale() / window->nativeScale_;
    }
    else
    {
        return this->scale();
    }
}

void BaseWidget::childEvent(QChildEvent *event)
{
    if (event->added())
    {
        // add element if it's a basewidget
        if (auto widget = dynamic_cast<BaseWidget *>(event->child()))
        {
            this->widgets_.push_back(widget);
        }
    }
    else if (event->removed())
    {
        // find element to be removed
        auto it = std::find_if(this->widgets_.begin(), this->widgets_.end(),
                               [&](auto &&x) { return x == event->child(); });

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

}  // namespace AB_NAMESPACE
