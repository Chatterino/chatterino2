#include "widgets/basewidget.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QDebug>
#include <QLayout>
#include <QtGlobal>
#include <boost/signals2.hpp>
#include "util/nativeeventhelper.hpp"

namespace chatterino {
namespace widgets {

BaseWidget::BaseWidget(singletons::ThemeManager &_themeManager, QWidget *parent)
    : QWidget(parent)
    , themeManager(_themeManager)
{
    this->init();
}

BaseWidget::BaseWidget(BaseWidget *parent)
    : QWidget(parent)
    , themeManager(singletons::ThemeManager::getInstance())
{
    this->init();
}

BaseWidget::BaseWidget(QWidget *parent)
    : QWidget(parent)
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

void BaseWidget::initAsWindow()
{
    this->isWindow = true;

#ifdef USEWINSDK
    auto dpi = util::getWindowDpi(this->winId());

    if (dpi) {
        this->dpiMultiplier = dpi.value() / 96.f;
    }
#endif

    if (singletons::SettingManager::getInstance().windowTopMost.getValue()) {
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
    }
}

void BaseWidget::refreshTheme()
{
    // Do any color scheme updates here
}

#ifdef USEWINSDK
bool BaseWidget::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    int dpi;

    if (util::tryHandleDpiChangedMessage(message, dpi)) {
        qDebug() << "dpi changed";

        float oldDpiMultiplier = this->dpiMultiplier;
        this->dpiMultiplier = dpi / 96.f;
        float scale = this->dpiMultiplier / oldDpiMultiplier;

        this->dpiMultiplierChanged(oldDpiMultiplier, this->dpiMultiplier);

        this->resize(static_cast<int>(this->width() * scale),
                     static_cast<int>(this->height() * scale));
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

void BaseWidget::changeEvent(QEvent *)
{
    if (this->isWindow) {
        TooltipWidget::getInstance()->hide();
    }
}

void BaseWidget::leaveEvent(QEvent *)
{
    if (this->isWindow) {
        TooltipWidget::getInstance()->hide();
    }
}
}  // namespace widgets
}  // namespace chatterino
