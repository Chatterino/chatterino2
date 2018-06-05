#include "widgets/basewidget.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"

#include <QChildEvent>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QtGlobal>

namespace chatterino {
namespace widgets {

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
    BaseWidget *baseWidget = dynamic_cast<BaseWidget *>(this->window());

    if (baseWidget == nullptr) {
        return 1.f;
    }

    return baseWidget->scale;
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

    this->scaleChangedEvent(value);
    this->scaleChanged.invoke(value);

    this->setScaleIndependantSize(this->getScaleIndependantSize());

    // set scale for all children
    BaseWidget::setScaleRecursive(value, this);
}

void BaseWidget::setScaleRecursive(float scale, QObject *object)
{
    for (QObject *child : object->children()) {
        BaseWidget *widget = dynamic_cast<BaseWidget *>(child);
        if (widget != nullptr) {
            widget->setScale(scale);
            continue;
        }

        //        QLayout *layout = nullptr;
        //        QWidget *widget = dynamic_cast<QWidget *>(child);

        //        if (widget != nullptr) {
        //            layout = widget->layout();
        //        }

        //        else {
        QLayout *layout = dynamic_cast<QLayout *>(object);

        if (layout != nullptr) {
            setScaleRecursive(scale, layout);
        }
        //        }
    }
}

void BaseWidget::scaleChangedEvent(float newDpi)
{
}

void BaseWidget::themeRefreshEvent()
{
    // Do any color scheme updates here
}

}  // namespace widgets
}  // namespace chatterino
