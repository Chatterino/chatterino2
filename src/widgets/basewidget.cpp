#include "widgets/basewidget.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"

#include <QChildEvent>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QtGlobal>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

BaseWidget::BaseWidget(singletons::ThemeManager &_themeManager, QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , themeManager(_themeManager)
{
    this->init();
}

BaseWidget::BaseWidget(BaseWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , themeManager(singletons::ThemeManager::getInstance())
{
    this->init();
}

float BaseWidget::getScale() const
{
    //    return 1.f;
    BaseWidget *baseWidget = dynamic_cast<BaseWidget *>(this->window());

    if (baseWidget == nullptr) {
        return 1.f;
    } else {
        return baseWidget->scale;
    }
}

void BaseWidget::init()
{
    auto connection = this->themeManager.updated.connect([this]() {
        this->themeRefreshEvent();

        this->update();
    });

    QObject::connect(this, &QObject::destroyed, [connection] {
        connection.disconnect();  //
    });
}

void BaseWidget::childEvent(QChildEvent *event)
{
    if (event->added()) {
        BaseWidget *widget = dynamic_cast<BaseWidget *>(event->child());

        if (widget) {
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
void BaseWidget::setScale(float value)
{
    // update scale value
    this->scale = value;

    this->scaleChangedEvent(value);
    this->scaleChanged.invoke(value);

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
