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
    this->init();
}

BaseWidget::~BaseWidget()
{
    this->themeConnection.disconnect();
}

float BaseWidget::getScale() const
{
    if (this->overrideScale) {
        return this->overrideScale.get();
    }

    BaseWidget *baseWidget = dynamic_cast<BaseWidget *>(this->window());

    if (baseWidget == nullptr) {
        return 1.f;
    }

    return baseWidget->scale;
}

void BaseWidget::setOverrideScale(boost::optional<float> value)
{
    this->overrideScale = value;
    this->setScale(this->getScale());
}

boost::optional<float> BaseWidget::getOverrideScale() const
{
    return this->overrideScale;
}

QSize BaseWidget::getScaleIndependantSize() const
{
    return this->scaleIndependantSize;
}

int BaseWidget::getScaleIndependantWidth() const
{
    return this->scaleIndependantSize.width();
}

int BaseWidget::getScaleIndependantHeight() const
{
    return this->scaleIndependantSize.height();
}

void BaseWidget::setScaleIndependantSize(int width, int height)
{
    this->setScaleIndependantSize(QSize(width, height));
}

void BaseWidget::setScaleIndependantSize(QSize size)
{
    this->scaleIndependantSize = size;

    if (size.width() > 0) {
        this->setFixedWidth((int)(size.width() * this->getScale()));
    }
    if (size.height() > 0) {
        this->setFixedHeight((int)(size.height() * this->getScale()));
    }
}

void BaseWidget::setScaleIndependantWidth(int value)
{
    this->setScaleIndependantSize(QSize(value, this->scaleIndependantSize.height()));
}

void BaseWidget::setScaleIndependantHeight(int value)
{
    this->setScaleIndependantSize(QSize(this->scaleIndependantSize.height(), value));
}

void BaseWidget::init()
{
    auto app = getApp();
    this->themeManager = app->themes;

    this->themeConnection = this->themeManager->updated.connect([this]() {
        this->themeRefreshEvent();

        this->update();
    });
}

void BaseWidget::childEvent(QChildEvent *event)
{
    if (event->added()) {
        BaseWidget *widget = dynamic_cast<BaseWidget *>(event->child());

        if (widget != nullptr) {
            this->widgets.push_back(widget);
        }
    } else if (event->removed()) {
        for (auto it = this->widgets.begin(); it != this->widgets.end(); it++) {
            if (*it == event->child()) {
                this->widgets.erase(it);
                break;
            }
        }
    }
}

void BaseWidget::showEvent(QShowEvent *)
{
    this->setScale(this->getScale());
    this->themeRefreshEvent();
}

void BaseWidget::setScale(float value)
{
    // update scale value
    this->scale = value;

    this->scaleChangedEvent(this->getScale());
    this->scaleChanged.invoke(this->getScale());

    this->setScaleIndependantSize(this->getScaleIndependantSize());
}

void BaseWidget::scaleChangedEvent(float newDpi)
{
}

void BaseWidget::themeRefreshEvent()
{
    // Do any color scheme updates here
}

}  // namespace chatterino
