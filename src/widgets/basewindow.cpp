#include "basewindow.hpp"

#include "singletons/settingsmanager.hpp"
#include "util/nativeeventhelper.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QApplication>
#include <QDebug>
#include <QIcon>

#ifdef USEWINSDK
#include <dwmapi.h>
#include <gdiplus.h>
#include <objidl.h>
#include <windows.h>
#include <windowsx.h>
#pragma comment(lib, "Dwmapi.lib")

#include <QHBoxLayout>
#include <QVBoxLayout>

#define WM_DPICHANGED 0x02E0
#endif

#include "widgets/helper/rippleeffectlabel.hpp"

namespace chatterino {
namespace widgets {

BaseWindow::BaseWindow(singletons::ThemeManager &_themeManager, QWidget *parent,
                       bool _enableCustomFrame)
    : BaseWidget(_themeManager, parent, Qt::Window)
    , enableCustomFrame(_enableCustomFrame)
{
    this->init();
}

BaseWindow::BaseWindow(BaseWidget *parent, bool _enableCustomFrame)
    : BaseWidget(parent, Qt::Window)
    , enableCustomFrame(_enableCustomFrame)
{
    this->init();
}

BaseWindow::BaseWindow(QWidget *parent, bool _enableCustomFrame)
    : BaseWidget(parent, Qt::Window)
    , enableCustomFrame(_enableCustomFrame)
{
    this->init();
}

void BaseWindow::init()
{
    this->setWindowIcon(QIcon(":/images/icon.png"));

#ifdef USEWINSDK
    if (this->hasCustomWindowFrame()) {
        // CUSTOM WINDOW FRAME
        QVBoxLayout *layout = new QVBoxLayout;
        layout->setMargin(1);
        this->setLayout(layout);
        {
            QHBoxLayout *buttons = this->titlebarBox = new QHBoxLayout;
            buttons->setMargin(0);
            layout->addLayout(buttons);

            // title
            QLabel *titleLabel = new QLabel("Chatterino");
            buttons->addWidget(titleLabel);
            this->titleLabel = titleLabel;

            // buttons
            RippleEffectLabel *min = new RippleEffectLabel;
            min->getLabel().setText("min");
            min->setFixedSize(46, 30);
            RippleEffectLabel *max = new RippleEffectLabel;
            max->setFixedSize(46, 30);
            max->getLabel().setText("max");
            RippleEffectLabel *exit = new RippleEffectLabel;
            exit->setFixedSize(46, 30);
            exit->getLabel().setText("exit");

            this->minButton = min;
            this->maxButton = max;
            this->exitButton = exit;

            this->widgets.push_back(min);
            this->widgets.push_back(max);
            this->widgets.push_back(exit);

            buttons->addStretch(1);
            buttons->addWidget(min);
            buttons->addWidget(max);
            buttons->addWidget(exit);
        }
        this->layoutBase = new QWidget(this);
        this->widgets.push_back(this->layoutBase);
        layout->addWidget(this->layoutBase);
    }

    // DPI
    auto dpi = util::getWindowDpi(this->winId());

    if (dpi) {
        this->dpiMultiplier = dpi.value() / 96.f;
    }

    this->dpiMultiplierChanged(1, this->dpiMultiplier);
#endif

    if (singletons::SettingManager::getInstance().windowTopMost.getValue()) {
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
    }
}

QWidget *BaseWindow::getLayoutContainer()
{
    if (this->hasCustomWindowFrame()) {
        return this->layoutBase;
    } else {
        return this;
    }
}

bool BaseWindow::hasCustomWindowFrame()
{
#ifdef Q_OS_WIN
    //    return this->enableCustomFrame;
    return false;
#else
    return false;
#endif
}

void BaseWindow::refreshTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Background, this->themeManager.windowBg);
    palette.setColor(QPalette::Foreground, this->themeManager.windowText);
    this->setPalette(palette);
}

void BaseWindow::addTitleBarButton(const QString &text)
{
    RippleEffectLabel *label = new RippleEffectLabel;
    label->getLabel().setText(text);
    this->widgets.push_back(label);
    this->titlebarBox->insertWidget(2, label);
}

void BaseWindow::changeEvent(QEvent *)
{
    TooltipWidget::getInstance()->hide();
}

void BaseWindow::leaveEvent(QEvent *)
{
    TooltipWidget::getInstance()->hide();
}

#ifdef USEWINSDK
bool BaseWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG *msg = reinterpret_cast<MSG *>(message);

    switch (msg->message) {
        case WM_DPICHANGED: {
            qDebug() << "dpi changed";
            int dpi = HIWORD(msg->wParam);

            float oldDpiMultiplier = this->dpiMultiplier;
            this->dpiMultiplier = dpi / 96.f;
            float scale = this->dpiMultiplier / oldDpiMultiplier;

            this->dpiMultiplierChanged(oldDpiMultiplier, this->dpiMultiplier);

            this->resize(static_cast<int>(this->width() * scale),
                         static_cast<int>(this->height() * scale));

            return true;
        }
        case WM_NCCALCSIZE: {
            if (this->hasCustomWindowFrame()) {
                // this kills the window frame and title bar we added with
                // WS_THICKFRAME and WS_CAPTION
                *result = 0;
                return true;
            } else {
                return QWidget::nativeEvent(eventType, message, result);
            }
            break;
        }
        case WM_NCHITTEST: {
            if (this->hasCustomWindowFrame()) {
                *result = 0;
                const LONG border_width = 8;  // in pixels
                RECT winrect;
                GetWindowRect((HWND)winId(), &winrect);

                long x = GET_X_LPARAM(msg->lParam);
                long y = GET_Y_LPARAM(msg->lParam);

                bool resizeWidth = minimumWidth() != maximumWidth();
                bool resizeHeight = minimumHeight() != maximumHeight();

                if (resizeWidth) {
                    // left border
                    if (x >= winrect.left && x < winrect.left + border_width) {
                        *result = HTLEFT;
                    }
                    // right border
                    if (x < winrect.right && x >= winrect.right - border_width) {
                        *result = HTRIGHT;
                    }
                }
                if (resizeHeight) {
                    // bottom border
                    if (y < winrect.bottom && y >= winrect.bottom - border_width) {
                        *result = HTBOTTOM;
                    }
                    // top border
                    if (y >= winrect.top && y < winrect.top + border_width) {
                        *result = HTTOP;
                    }
                }
                if (resizeWidth && resizeHeight) {
                    // bottom left corner
                    if (x >= winrect.left && x < winrect.left + border_width &&
                        y < winrect.bottom && y >= winrect.bottom - border_width) {
                        *result = HTBOTTOMLEFT;
                    }
                    // bottom right corner
                    if (x < winrect.right && x >= winrect.right - border_width &&
                        y < winrect.bottom && y >= winrect.bottom - border_width) {
                        *result = HTBOTTOMRIGHT;
                    }
                    // top left corner
                    if (x >= winrect.left && x < winrect.left + border_width && y >= winrect.top &&
                        y < winrect.top + border_width) {
                        *result = HTTOPLEFT;
                    }
                    // top right corner
                    if (x < winrect.right && x >= winrect.right - border_width &&
                        y >= winrect.top && y < winrect.top + border_width) {
                        *result = HTTOPRIGHT;
                    }
                }

                if (*result == 0) {
                    bool client = false;

                    QPoint point(x - winrect.left, y - winrect.top);
                    for (QWidget *widget : this->widgets) {
                        if (widget->geometry().contains(point)) {
                            client = true;
                        }
                    }

                    if (client) {
                        *result = HTCLIENT;
                    } else {
                        *result = HTCAPTION;
                    }
                }

                qDebug() << *result;

                return true;
            } else {
                return QWidget::nativeEvent(eventType, message, result);
            }
            break;
        }  // end case WM_NCHITTEST
        case WM_CLOSE: {
            if (this->enableCustomFrame) {
                return close();
            }
            break;
        }
        default:
            return QWidget::nativeEvent(eventType, message, result);
    }
}

void BaseWindow::showEvent(QShowEvent *)
{
    if (this->isVisible() && this->hasCustomWindowFrame()) {
        SetWindowLongPtr((HWND)this->winId(), GWL_STYLE,
                         WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

        const MARGINS shadow = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea((HWND)this->winId(), &shadow);

        SetWindowPos((HWND)this->winId(), 0, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    }
}

void BaseWindow::paintEvent(QPaintEvent *event)
{
    BaseWidget::paintEvent(event);

    if (this->hasCustomWindowFrame()) {
        QPainter painter(this);

        bool windowFocused = this->window() == QApplication::activeWindow();

        if (windowFocused) {
            painter.setPen(this->themeManager.tabs.selected.backgrounds.regular.color());
        } else {
            painter.setPen(this->themeManager.tabs.selected.backgrounds.unfocused.color());
        }
        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }
}
#endif

}  // namespace widgets
}  // namespace chatterino
