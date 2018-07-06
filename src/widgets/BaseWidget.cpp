#include "widgets/BaseWidget.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "widgets/BaseWindow.hpp"

#include <QChildEvent>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QtGlobal>

namespace chatterino {

BaseWidget::BaseWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
{
    this->theme = getApp()->themes;

    this->signalHolder_.managedConnect(this->theme->updated, [this]() {
        this->themeChangedEvent();

        this->update();
    });
}

float BaseWidget::getScale() const
{
    if (this->overrideScale_) {
        return this->overrideScale_.get();
    }

    BaseWidget *baseWidget = dynamic_cast<BaseWidget *>(this->window());

    if (baseWidget == nullptr) {
        return 1.f;
    }

    return baseWidget->scale_;
}

void BaseWidget::setScale(float value)
{
    // update scale value
    this->scale_ = value;

    this->scaleChangedEvent(this->getScale());
    this->scaleChanged.invoke(this->getScale());

    this->setScaleIndependantSize(this->getScaleIndependantSize());
}

void BaseWidget::setOverrideScale(boost::optional<float> value)
{
    this->overrideScale_ = value;
    this->setScale(this->getScale());
}

boost::optional<float> BaseWidget::getOverrideScale() const
{
    return this->overrideScale_;
}

QSize BaseWidget::getScaleIndependantSize() const
{
    return this->scaleIndependantSize_;
}

int BaseWidget::getScaleIndependantWidth() const
{
    return this->scaleIndependantSize_.width();
}

int BaseWidget::getScaleIndependantHeight() const
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

    if (size.width() > 0) {
        this->setFixedWidth((int)(size.width() * this->getScale()));
    }
    if (size.height() > 0) {
        this->setFixedHeight((int)(size.height() * this->getScale()));
    }
}

void BaseWidget::setScaleIndependantWidth(int value)
{
    this->setScaleIndependantSize(QSize(value, this->scaleIndependantSize_.height()));
}

void BaseWidget::setScaleIndependantHeight(int value)
{
    this->setScaleIndependantSize(QSize(this->scaleIndependantSize_.height(), value));
}

void BaseWidget::childEvent(QChildEvent *event)
{
    if (event->added()) {
        BaseWidget *widget = dynamic_cast<BaseWidget *>(event->child());

        if (widget != nullptr) {
            this->widgets_.push_back(widget);
        }
    } else if (event->removed()) {
        for (auto it = this->widgets_.begin(); it != this->widgets_.end(); it++) {
            if (*it == event->child()) {
                this->widgets_.erase(it);
                break;
            }
        }
    }
}

void BaseWidget::showEvent(QShowEvent *)
{
    this->setScale(this->getScale());
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
