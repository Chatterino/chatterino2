#include "basewindow.hpp"

#include "application.hpp"
#include "boost/algorithm/algorithm.hpp"
#include "debug/log.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/windowmanager.hpp"
#include "util/nativeeventhelper.hpp"
#include "util/posttothread.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"
#include "widgets/helper/shortcut.hpp"
#include "widgets/label.hpp"
#include "widgets/tooltipwidget.hpp"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QIcon>

#ifdef USEWINSDK
#include <ObjIdl.h>
#include <VersionHelpers.h>
#include <Windows.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <windowsx.h>
#pragma comment(lib, "Dwmapi.lib")

#include <QHBoxLayout>
#include <QVBoxLayout>

#define WM_DPICHANGED 0x02E0
#endif

#include "widgets/helper/titlebarbutton.hpp"

namespace chatterino {
namespace widgets {

BaseWindow::BaseWindow(QWidget *parent, Flags _flags)
    : BaseWidget(parent,
                 Qt::Window | ((_flags & TopMost) ? Qt::WindowStaysOnTopHint : Qt::WindowFlags()))
    , enableCustomFrame_(_flags & EnableCustomFrame)
    , frameless_(_flags & Frameless)
    , flags_(_flags)
{
    if (this->frameless_) {
        this->enableCustomFrame_ = false;
        this->setWindowFlag(Qt::FramelessWindowHint);
    }

    if (this->flags_ & DeleteOnFocusOut) {
        this->setAttribute(Qt::WA_DeleteOnClose);
    }

    this->init();

    this->connections_.managedConnect(
        getApp()->settings->uiScale.getValueChangedSignal(),
        [this](auto, auto) { util::postToThread([this] { this->updateScale(); }); });

    this->updateScale();

    CreateWindowShortcut(this, "CTRL+0", [] { getApp()->settings->uiScale.setValue(0); });
}

BaseWindow::Flags BaseWindow::getFlags()
{
    return this->flags_;
}

void BaseWindow::init()
{
    this->setWindowIcon(QIcon(":/images/icon.png"));

#ifdef USEWINSDK
    if (this->hasCustomWindowFrame()) {
        // CUSTOM WINDOW FRAME
        QVBoxLayout *layout = new QVBoxLayout();
        layout->setContentsMargins(0, 1, 0, 0);
        layout->setSpacing(0);
        this->setLayout(layout);
        {
            if (!this->frameless_) {
                QHBoxLayout *buttonLayout = this->ui_.titlebarBox = new QHBoxLayout();
                buttonLayout->setMargin(0);
                layout->addLayout(buttonLayout);

                // title
                Label *title = new Label("Chatterino");
                QObject::connect(this, &QWidget::windowTitleChanged,
                                 [title](const QString &text) { title->setText(text); });

                QSizePolicy policy(QSizePolicy::Ignored, QSizePolicy::Preferred);
                policy.setHorizontalStretch(1);
                //            title->setBaseSize(0, 0);
                //                title->setScaledContents(true);
                title->setSizePolicy(policy);
                buttonLayout->addWidget(title);
                this->ui_.titleLabel = title;

                // buttons
                TitleBarButton *_minButton = new TitleBarButton;
                _minButton->setButtonStyle(TitleBarButton::Minimize);
                TitleBarButton *_maxButton = new TitleBarButton;
                _maxButton->setButtonStyle(TitleBarButton::Maximize);
                TitleBarButton *_exitButton = new TitleBarButton;
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

                this->ui_.minButton = _minButton;
                this->ui_.maxButton = _maxButton;
                this->ui_.exitButton = _exitButton;

                this->ui_.buttons.push_back(_minButton);
                this->ui_.buttons.push_back(_maxButton);
                this->ui_.buttons.push_back(_exitButton);

                //            buttonLayout->addStretch(1);
                buttonLayout->addWidget(_minButton);
                buttonLayout->addWidget(_maxButton);
                buttonLayout->addWidget(_exitButton);
                buttonLayout->setSpacing(0);
            }
        }
        this->ui_.layoutBase = new BaseWidget(this);
        layout->addWidget(this->ui_.layoutBase);
    }

    // DPI
    auto dpi = util::getWindowDpi(this->winId());

    if (dpi) {
        this->scale = dpi.value() / 96.f;
    }
#endif

#ifdef USEWINSDK
    // fourtf: don't ask me why we need to delay this
    if (!(this->flags_ & Flags::TopMost)) {
        QTimer::singleShot(1, this, [this] {
            getApp()->settings->windowTopMost.connect([this](bool topMost, auto) {
                ::SetWindowPos(HWND(this->winId()), topMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0,
                               0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            });
        });
    }
#else
//    if (getApp()->settings->windowTopMost.getValue()) {
//        this->setWindowFlag(Qt::WindowStaysOnTopHint);
//    }
#endif
}

void BaseWindow::setStayInScreenRect(bool value)
{
    this->stayInScreenRect_ = value;
}

bool BaseWindow::getStayInScreenRect() const
{
    return this->stayInScreenRect_;
}

QWidget *BaseWindow::getLayoutContainer()
{
    if (this->hasCustomWindowFrame()) {
        return this->ui_.layoutBase;
    } else {
        return this;
    }
}

bool BaseWindow::hasCustomWindowFrame()
{
#ifdef USEWINSDK
    static bool isWin8 = IsWindows8OrGreater();

    return isWin8 && this->enableCustomFrame_;
#else
    return false;
#endif
}

void BaseWindow::themeRefreshEvent()
{
    if (this->hasCustomWindowFrame()) {
        QPalette palette;
        palette.setColor(QPalette::Background, QColor(0, 0, 0, 0));
        palette.setColor(QPalette::Foreground, this->themeManager->window.text);
        this->setPalette(palette);

        if (this->ui_.titleLabel) {
            QPalette palette_title;
            palette_title.setColor(QPalette::Foreground,
                                   this->themeManager->isLightTheme() ? "#333" : "#ccc");
            this->ui_.titleLabel->setPalette(palette_title);
        }

        for (RippleEffectButton *button : this->ui_.buttons) {
            button->setMouseEffectColor(this->themeManager->window.text);
        }
    } else {
        QPalette palette;
        palette.setColor(QPalette::Background, this->themeManager->window.background);
        palette.setColor(QPalette::Foreground, this->themeManager->window.text);
        this->setPalette(palette);
    }
}

bool BaseWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate /*|| event->type() == QEvent::FocusOut*/) {
        if (this->flags_ & DeleteOnFocusOut) {
            this->close();
        }
    }

    return QWidget::event(event);
}

void BaseWindow::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->delta() > 0) {
            getApp()->settings->uiScale.setValue(singletons::WindowManager::clampUiScale(
                getApp()->settings->uiScale.getValue() + 1));
        } else {
            getApp()->settings->uiScale.setValue(singletons::WindowManager::clampUiScale(
                getApp()->settings->uiScale.getValue() - 1));
        }
    }
}

void BaseWindow::addTitleBarButton(const TitleBarButton::Style &style,
                                   std::function<void()> onClicked)
{
    TitleBarButton *button = new TitleBarButton;
    button->setScaleIndependantSize(30, 30);

    this->ui_.buttons.push_back(button);
    this->ui_.titlebarBox->insertWidget(1, button);
    button->setButtonStyle(style);

    QObject::connect(button, &TitleBarButton::clicked, this, [onClicked] { onClicked(); });
}

RippleEffectLabel *BaseWindow::addTitleBarLabel(std::function<void()> onClicked)
{
    RippleEffectLabel *button = new RippleEffectLabel;
    button->setScaleIndependantHeight(30);

    this->ui_.buttons.push_back(button);
    this->ui_.titlebarBox->insertWidget(1, button);

    QObject::connect(button, &RippleEffectLabel::clicked, this, [onClicked] { onClicked(); });

    return button;
}

void BaseWindow::changeEvent(QEvent *)
{
    TooltipWidget::getInstance()->hide();

#ifdef USEWINSDK
    if (this->ui_.maxButton) {
        this->ui_.maxButton->setButtonStyle(this->windowState() & Qt::WindowMaximized
                                                ? TitleBarButton::Unmaximize
                                                : TitleBarButton::Maximize);
    }
#endif

#ifndef Q_OS_WIN
    this->update();
#endif
}

void BaseWindow::leaveEvent(QEvent *)
{
    TooltipWidget::getInstance()->hide();
}

void BaseWindow::moveTo(QWidget *parent, QPoint point, bool offset)
{
    if (offset) {
        point.rx() += 16;
        point.ry() += 16;
    }

    this->move(point);
    this->moveIntoDesktopRect(parent);
}

void BaseWindow::resizeEvent(QResizeEvent *)
{
    this->moveIntoDesktopRect(this);

    this->calcButtonsSizes();
}

void BaseWindow::moveIntoDesktopRect(QWidget *parent)
{
    if (!this->stayInScreenRect_)
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
            int dpi = HIWORD(msg->wParam);

            float oldScale = this->scale;
            float _scale = dpi / 96.f;
            float resizeScale = _scale / oldScale;

            this->resize(static_cast<int>(this->width() * resizeScale),
                         static_cast<int>(this->height() * resizeScale));

            this->nativeScale_ = _scale;
            this->updateScale();

            return true;
        }
        case WM_NCCALCSIZE: {
            if (this->hasCustomWindowFrame()) {
                int cx = GetSystemMetrics(SM_CXSIZEFRAME);
                int cy = GetSystemMetrics(SM_CYSIZEFRAME);

                if (msg->wParam == TRUE) {
                    NCCALCSIZE_PARAMS *ncp = (reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam));
                    ncp->lppos->flags |= SWP_NOREDRAW;
                    RECT *clientRect = &ncp->rgrc[0];

                    if (IsWindows10OrGreater()) {
                        clientRect->left += cx;
                        clientRect->top += 0;
                        clientRect->right -= cx;
                        clientRect->bottom -= cy;
                    } else {
                        clientRect->left += 1;
                        clientRect->top += 0;
                        clientRect->right -= 1;
                        clientRect->bottom -= 1;
                    }
                }

                *result = 0;
                return true;
            } else {
                return QWidget::nativeEvent(eventType, message, result);
            }
        } break;
        case WM_NCHITTEST: {
            if (this->hasCustomWindowFrame()) {
                *result = 0;
                const LONG border_width = 8;  // in pixels
                RECT winrect;
                GetWindowRect(HWND(winId()), &winrect);

                long x = GET_X_LPARAM(msg->lParam);
                long y = GET_Y_LPARAM(msg->lParam);

                bool resizeWidth = minimumWidth() != maximumWidth();
                bool resizeHeight = minimumHeight() != maximumHeight();

                if (resizeWidth) {
                    // left border
                    if (x < winrect.left + border_width) {
                        *result = HTLEFT;
                    }
                    // right border
                    if (x >= winrect.right - border_width) {
                        *result = HTRIGHT;
                    }
                }
                if (resizeHeight) {
                    // bottom border
                    if (y >= winrect.bottom - border_width) {
                        *result = HTBOTTOM;
                    }
                    // top border
                    if (y < winrect.top + border_width) {
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
                    for (QWidget *widget : this->ui_.buttons) {
                        if (widget->geometry().contains(point)) {
                            client = true;
                        }
                    }

                    if (this->ui_.layoutBase->geometry().contains(point)) {
                        client = true;
                    }

                    if (client) {
                        *result = HTCLIENT;
                    } else {
                        *result = HTCAPTION;
                    }
                }

                return true;
            } else {
                return QWidget::nativeEvent(eventType, message, result);
            }
            break;
        }
        default:
            return QWidget::nativeEvent(eventType, message, result);
    }
}

void BaseWindow::showEvent(QShowEvent *event)
{
    if (!this->shown_ && this->isVisible() && this->hasCustomWindowFrame()) {
        this->shown_ = true;
        //        SetWindowLongPtr((HWND)this->winId(), GWL_STYLE,
        //                         WS_POPUP | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX |
        //                         WS_MINIMIZEBOX);

        const MARGINS shadow = {8, 8, 8, 8};
        DwmExtendFrameIntoClientArea(HWND(this->winId()), &shadow);
    }

    BaseWidget::showEvent(event);
}

void BaseWindow::scaleChangedEvent(float)
{
    this->calcButtonsSizes();
}
#endif

void BaseWindow::paintEvent(QPaintEvent *)
{
    if (this->frameless_) {
        QPainter painter(this);

        painter.setPen(QColor("#999"));
        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }

#ifdef USEWINSDK
    if (this->hasCustomWindowFrame()) {
        QPainter painter(this);

        //        bool windowFocused = this->window() == QApplication::activeWindow();

        painter.fillRect(QRect(0, 1, this->width() - 0, this->height() - 0),
                         this->themeManager->window.background);
    }
#endif
}

void BaseWindow::updateScale()
{
    this->setScale(this->nativeScale_ * (this->flags_ & DisableCustomScaling
                                             ? 1
                                             : getApp()->windows->getUiScaleValue()));
}

void BaseWindow::calcButtonsSizes()
{
    if (!this->shown_) {
        return;
    }
    if ((this->width() / this->getScale()) < 300) {
        if (this->ui_.minButton)
            this->ui_.minButton->setScaleIndependantSize(30, 30);
        if (this->ui_.maxButton)
            this->ui_.maxButton->setScaleIndependantSize(30, 30);
        if (this->ui_.exitButton)
            this->ui_.exitButton->setScaleIndependantSize(30, 30);
    } else {
        if (this->ui_.minButton)
            this->ui_.minButton->setScaleIndependantSize(46, 30);
        if (this->ui_.maxButton)
            this->ui_.maxButton->setScaleIndependantSize(46, 30);
        if (this->ui_.exitButton)
            this->ui_.exitButton->setScaleIndependantSize(46, 30);
    }
}
}  // namespace widgets
}  // namespace chatterino
