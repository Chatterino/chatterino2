#include "widgets/basewidget.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"

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

BaseWidget::BaseWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f)
    , themeManager(singletons::ThemeManager::getInstance())
{
}

float BaseWidget::getDpiMultiplier()
{
    //    return 1.f;
    BaseWidget *baseWidget = dynamic_cast<BaseWidget *>(this->window());

    if (baseWidget == nullptr) {
        return 1.f;
    } else {
        return baseWidget->dpiMultiplier;
        //    int screenNr = QApplication::desktop()->screenNumber(this);
        //    QScreen *screen = QApplication::screens().at(screenNr);
        //    return screen->logicalDotsPerInch() / 96.f;
    }
}

void BaseWidget::init()
{
    auto connection = this->themeManager.updated.connect([this]() {
        this->refreshTheme();

        this->update();
    });

    QObject::connect(this, &QObject::destroyed, [connection] {
        connection.disconnect();  //
    });
}

void BaseWidget::refreshTheme()
{
    // Do any color scheme updates here
}

}  // namespace widgets
}  // namespace chatterino
