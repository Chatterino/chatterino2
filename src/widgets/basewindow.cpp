#include "basewindow.hpp"

#include "singletons/settingsmanager.hpp"
#include "util/nativeeventhelper.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
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

#include "widgets/helper/titlebarbutton.hpp"

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
    : BaseWidget(singletons::ThemeManager::getInstance(), parent, Qt::Window)
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
        QVBoxLayout *layout = new QVBoxLayout();
        layout->setMargin(1);
        layout->setSpacing(0);
        this->setLayout(layout);
        {
            QHBoxLayout *buttonLayout = this->titlebarBox = new QHBoxLayout();
            buttonLayout->setMargin(0);
            layout->addLayout(buttonLayout);

            // title
            QLabel *title = new QLabel("   Chatterino");
            buttonLayout->addWidget(title);
            this->titleLabel = title;

            // buttons
            TitleBarButton *_minButton = new TitleBarButton;
            _minButton->setFixedSize(46, 30);
            _minButton->setButtonStyle(TitleBarButton::Minimize);
            TitleBarButton *_maxButton = new TitleBarButton;
            _maxButton->setFixedSize(46, 30);
            _maxButton->setButtonStyle(TitleBarButton::Maximize);
            TitleBarButton *_exitButton = new TitleBarButton;
            _exitButton->setFixedSize(46, 30);
            _exitButton->setButtonStyle(TitleBarButton::Close);

            QObject::connect(_minButton, &TitleBarButton::clicked, this, [this] {
                this->setWindowState(Qt::WindowMinimized | this->windowState());
            });
            QObject::connect(_maxButton, &TitleBarButton::clicked, this, [this] {
                this->setWindowState(this->windowState() == Qt::WindowMaximized
                                         ? Qt::WindowActive
                                         : Qt::WindowMaximized);
            });
            QObject::connect(_exitButton, &TitleBarButton::clicked, this,
                             [this] { this->close(); });

            this->minButton = _minButton;
            this->maxButton = _maxButton;
            this->exitButton = _exitButton;

            this->buttons.push_back(_minButton);
            this->buttons.push_back(_maxButton);
            this->buttons.push_back(_exitButton);

            buttonLayout->addStretch(1);
            buttonLayout->addWidget(_minButton);
            buttonLayout->addWidget(_maxButton);
            buttonLayout->addWidget(_exitButton);
            buttonLayout->setSpacing(0);
        }
        this->layoutBase = new BaseWidget(this);
        layout->addWidget(this->layoutBase);
    }

    // DPI
    auto dpi = util::getWindowDpi(this->winId());

    if (dpi) {
        this->scale = dpi.value() / 96.f;
    }

    this->scaleChangedEvent(this->scale);
#endif

    if (singletons::SettingManager::getInstance().windowTopMost.getValue()) {
        this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
    }
}

void BaseWindow::setStayInScreenRect(bool value)
{
    this->stayInScreenRect = value;
}

bool BaseWindow::getStayInScreenRect() const
{
    return this->stayInScreenRect;
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
    return this->enableCustomFrame;
//    return false;
#else
    return false;
#endif
}

void BaseWindow::themeRefreshEvent()
{
    QPalette palette;
    palette.setColor(QPalette::Background, this->themeManager.windowBg);
    palette.setColor(QPalette::Foreground, this->themeManager.windowText);
    this->setPalette(palette);

    for (RippleEffectButton *button : this->buttons) {
        button->setMouseEffectColor(this->themeManager.windowText);
    }
}

void BaseWindow::addTitleBarButton(const TitleBarButton::Style &style,
                                   std::function<void()> onClicked)
{
    TitleBarButton *button = new TitleBarButton;

    this->buttons.push_back(button);
    this->titlebarBox->insertWidget(2, button);
    button->setButtonStyle(style);

    QObject::connect(button, &TitleBarButton::clicked, this, [onClicked] { onClicked(); });
}

void BaseWindow::changeEvent(QEvent *)
{
    TooltipWidget::getInstance()->hide();

#ifdef USEWINSDK
    if (this->hasCustomWindowFrame()) {
        this->maxButton->setButtonStyle(this->windowState() & Qt::WindowMaximized
                                            ? TitleBarButton::Unmaximize
                                            : TitleBarButton::Maximize);
    }
#endif
}

void BaseWindow::leaveEvent(QEvent *)
{
    TooltipWidget::getInstance()->hide();
}

void BaseWindow::moveTo(QWidget *parent, QPoint point)
{
    point.rx() += 16;
    point.ry() += 16;

    this->move(point);
    this->moveIntoDesktopRect(parent);
}

void BaseWindow::resizeEvent(QResizeEvent *)
{
    this->moveIntoDesktopRect(this);
}

void BaseWindow::moveIntoDesktopRect(QWidget *parent)
{
    if (!this->stayInScreenRect)
        return;

    // move the widget into the screen geometry if it's not already in there
    QDesktopWidget *desktop = QApplication::desktop();

    QRect s = desktop->screenGeometry(parent);
    QPoint p = this->pos();

    if (p.x() < s.left()) {
        p.setX(s.left());
    }
    if (p.y() < s.top()) {
        p.setY(s.top());
    }
    if (p.x() + this->width() > s.right()) {
        p.setX(s.right() - this->width());
    }
    if (p.y() + this->height() > s.bottom()) {
        p.setY(s.bottom() - this->height());
    }

    if (p != this->pos())
        this->move(p);
}

#ifdef USEWINSDK
bool BaseWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    MSG *msg = reinterpret_cast<MSG *>(message);

    switch (msg->message) {
        case WM_DPICHANGED: {
            qDebug() << "dpi changed";
            int dpi = HIWORD(msg->wParam);

            float oldScale = this->scale;
            float _scale = dpi / 96.f;
            float resizeScale = _scale / oldScale;

            this->resize(static_cast<int>(this->width() * resizeScale),
                         static_cast<int>(this->height() * resizeScale));

            this->setScale(_scale);

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
                    for (QWidget *widget : this->buttons) {
                        if (widget->geometry().contains(point)) {
                            client = true;
                        }
                    }

                    if (this->layoutBase->geometry().contains(point)) {
                        client = true;
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
            //            if (this->enableCustomFrame) {
            //                this->close();
            //            }
            return QWidget::nativeEvent(eventType, message, result);
            break;
        }
        default:
            return QWidget::nativeEvent(eventType, message, result);
    }
}

void BaseWindow::showEvent(QShowEvent *event)
{
    if (!this->shown && this->isVisible() && this->hasCustomWindowFrame()) {
        this->shown = true;
        SetWindowLongPtr((HWND)this->winId(), GWL_STYLE,
                         WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);

        const MARGINS shadow = {1, 1, 1, 1};
        DwmExtendFrameIntoClientArea((HWND)this->winId(), &shadow);

        SetWindowPos((HWND)this->winId(), 0, 0, 0, 0, 0,
                     SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
    }

    BaseWidget::showEvent(event);
}

void BaseWindow::paintEvent(QPaintEvent *event)
{
    if (this->hasCustomWindowFrame()) {
        BaseWidget::paintEvent(event);

        QPainter painter(this);

        bool windowFocused = this->window() == QApplication::activeWindow();

        QLinearGradient gradient(0, 0, 10, 250);
        gradient.setColorAt(1, this->themeManager.tabs.selected.backgrounds.unfocused.color());

        if (windowFocused) {
            gradient.setColorAt(.4, this->themeManager.tabs.selected.backgrounds.regular.color());
        } else {
            gradient.setColorAt(.4, this->themeManager.tabs.selected.backgrounds.unfocused.color());
        }
        painter.setPen(QPen(QBrush(gradient), 1));

        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }
}
#endif

}  // namespace widgets
}  // namespace chatterino
