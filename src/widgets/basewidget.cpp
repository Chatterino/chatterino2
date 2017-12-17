#include "widgets/basewidget.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"

//#include <QApplication>
#include <QDebug>
//#include <QDesktopWidget>
//#include <QScreen>
#include <boost/signals2.hpp>
#include "util/nativeeventhelper.hpp"

namespace chatterino {
namespace widgets {

// BaseWidget::BaseWidget(ColorScheme &_colorScheme, WindowManager &_windowManager, QWidget *parent)
BaseWidget::BaseWidget(ColorScheme &_colorScheme, QWidget *parent)
    : QWidget(parent)
    , colorScheme(_colorScheme)
//    , windowManager(_windowManager)
{
    this->init();
}

BaseWidget::BaseWidget(BaseWidget *parent)
    : QWidget(parent)
    , colorScheme(parent->colorScheme)
//    , windowManager(parent->windowManager)
{
    this->init();
}

BaseWidget::BaseWidget(QWidget *parent)
    : QWidget(parent)
    , colorScheme(*ColorScheme::instance)
{
}

float BaseWidget::getDpiMultiplier()
{
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
    this->colorScheme.updated.connect([this]() {
        this->refreshTheme();

        this->update();
    });
}

void BaseWidget::initAsWindow()
{
#ifdef USEWINSDK
    auto dpi = util::getWindowDpi(this->winId());

    if (dpi) {
        this->dpiMultiplier = dpi.value() / 96.f;
    }
#endif

    if (SettingsManager::getInstance().windowTopMost.getValue()) {
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

        qDebug() << scale;
        qDebug() << dpi;

        this->resize(static_cast<int>(this->width() * scale),
                     static_cast<int>(this->height() * scale));
    }

    return QWidget::nativeEvent(eventType, message, result);
}
#endif

}  // namespace widgets
}  // namespace chatterino
