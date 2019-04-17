#include "BaseWindow.hpp"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <functional>

#include "ab/Column.hpp"
#include "ab/Row.hpp"
#include "ab/TitleBarButton.hpp"
#include "ab/util/MakeWidget.hpp"
#include "ab/util/ScaleQss.hpp"
#include "ab/util/WindowsHelper.hpp"

#ifdef USEWINSDK
#    include <ObjIdl.h>
#    include <VersionHelpers.h>
#    include <Windows.h>
#    include <dwmapi.h>
#    include <gdiplus.h>
#    include <windowsx.h>

#    pragma comment(lib, "Dwmapi.lib")

#    include <QHBoxLayout>
#    include <QVBoxLayout>

#    define WM_DPICHANGED 0x02E0
#endif

namespace ab
{
    inline Qt::WindowFlags convertFlags(BaseWindow::Flags flags)
    {
        return Qt::Window |
               ((flags.has(BaseWindow::TopMost)) ? Qt::WindowStaysOnTopHint
                                                 : Qt::WindowFlags());
    }

    BaseWindow::BaseWindow(Flags flags)
        : BaseWidget(nullptr, convertFlags(flags))
        , enableCustomFrame_(flags.has(EnableCustomFrame))
        , frameless_(flags.has(Frameless))
        , flags_(flags)
    {
        if (this->frameless_)
        {
            this->enableCustomFrame_ = false;
            this->setWindowFlag(Qt::FramelessWindowHint);
        }

        this->initLayout();

        this->resize(600, 400);

        if (this->ui_.titleLabel)
        {
            QObject::connect(this, &QWidget::windowTitleChanged,
                this->ui_.titleLabel, &QLabel::setText);
        }

        // set theme
        QFile file(":/style/window.qss");
        file.open(QIODevice::ReadOnly);
        this->setScalableQss(file.readAll());
    }

    float BaseWindow::scale() const
    {
        //    return this->overrideScale().value_or(this->scale_);
        return this->nativeScale_;
    }

    BaseWindow::Flags BaseWindow::getFlags()
    {
        return this->flags_;
    }

    void BaseWindow::initLayout()
    {
        this->setWindowIcon(QIcon(":/images/icon.png"));

        QWidget::setLayout(makeLayout<Column>(
            [&](auto x) {
                this->ui_.windowLayout = x;
                if (!this->frameless_)
                    x->setContentsMargins(0, 1, 0, 0);
            },
            {
                makeWidget<WindowContent>([&](auto x) {
                    x->setLayout(makeLayout<Column>(
                        [&](auto x) {
                            this->ui_.layoutBase = x;
                            x->setStretch(0, 0);
                        },
                        {
                            this->ui_.titlebar = this->makeCustomTitleBar(),
                        }));
                }),
            }));

#ifdef USEWINSDK
        // fourtf: don't ask me why we need to delay this
//    if (!(this->flags_ & Flags::TopMost))
//    {
//        QTimer::singleShot(1, this, [this] {
//            getSettings()->windowTopMost.connect(
//                [this](bool topMost, auto) {
//                    ::SetWindowPos(HWND(this->winId()),
//                                   topMost ? HWND_TOPMOST : HWND_NOTOPMOST, 0,
//                                   0, 0, 0,
//                                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
//                },
//                this->managedConnections_);
//        });
//    }
#else
//    if (getSettings()->windowTopMost.getValue()) {
//        this->setWindowFlag(Qt::WindowStaysOnTopHint);
//    }
#endif
    }

    void BaseWindow::initCustomWindowFrame()
    {
    }

    ab::Row* BaseWindow::makeCustomTitleBar()
    {
        if (!this->hasCustomWindowFrame())
            return nullptr;
        else
            return makeLayout<Row>([](auto x) { x->setObjectName("content"); },
                {
                    makeWidget<QLabel>([&](auto x) {
                        x->setTextFormat(Qt::PlainText);
                        x->setObjectName("title-label");
                        x->setSizePolicy(
                            QSizePolicy::Ignored, QSizePolicy::Preferred);
                        this->ui_.titleLabel = x;
                    }),
                    makeWidget<TitleBarButton>([&](auto x) {
                        x->setObjectName("minimize-button");
                        x->setType(TitleBarButtonType::Minimize);
                        x->setSizePolicy(
                            QSizePolicy::Minimum, QSizePolicy::Minimum);
                        this->ui_.buttons.push_back(x);
                        QObject::connect(
                            x, &FlatButton::leftClicked, this, [this]() {
                                this->setWindowState(
                                    Qt::WindowMinimized | this->windowState());
                            });
                    }),
                    makeWidget<TitleBarButton>([&](auto x) {
                        x->setObjectName("maximize-button");
                        x->setType(TitleBarButtonType::Maximize);
                        x->setSizePolicy(
                            QSizePolicy::Minimum, QSizePolicy::Minimum);
                        this->ui_.buttons.push_back(x);
                        QObject::connect(
                            x, &FlatButton::leftClicked, this, [this]() {
                                this->setWindowState(
                                    (this->windowState() & Qt::WindowMaximized)
                                        ? Qt::WindowActive
                                        : Qt::WindowMaximized);
                            });
                        this->ui_.maxButton = x;
                    }),
                    makeWidget<TitleBarButton>([&](auto x) {
                        x->setObjectName("close-button");
                        x->setType(TitleBarButtonType::Close);
                        x->setSizePolicy(
                            QSizePolicy::Minimum, QSizePolicy::Minimum);
                        this->ui_.buttons.push_back(x);
                        QObject::connect(x, &FlatButton::leftClicked, this,
                            [this]() { this->close(); });
                    }),
                });
    }

    void BaseWindow::setStayInScreenRect(bool value)
    {
        this->stayInScreenRect_ = value;

        this->moveIntoDesktopRect(this);
    }

    bool BaseWindow::getStayInScreenRect() const
    {
        return this->stayInScreenRect_;
    }

    void BaseWindow::setActionOnFocusLoss(ActionOnFocusLoss value)
    {
        this->actionOnFocusLoss_ = value;
    }

    BaseWindow::ActionOnFocusLoss BaseWindow::getActionOnFocusLoss() const
    {
        return this->actionOnFocusLoss_;
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

    void BaseWindow::addTitleBarButton(QWidget* widget)
    {
        assert(this->ui_.titlebar);

        this->ui_.titlebar->insertWidget(1, widget);
    }

    void BaseWindow::setCenterWidget(QWidget* widget)
    {
        assert(this->ui_.layoutBase);

        this->ui_.layoutBase->addWidget(widget, 1);
    }

    const QString& BaseWindow::scalableQss() const
    {
        return this->scalableQss_;
    }

    void BaseWindow::setScalableQss(const QString& value)
    {
        this->scalableQss_ = value;

        this->updateScalableQss();
    }

    void BaseWindow::setCenterLayout(QLayout* layout)
    {
        assert(this->ui_.layoutBase);

        this->ui_.layoutBase->addLayout(layout, 1);
    }

    void BaseWindow::setLayout(QLayout*)
    {
        assert(false);
    }

    void BaseWindow::removeBackground()
    {
        QPalette palette;
        palette.setColor(QPalette::Background, QColor(0, 0, 0, 0));
        // palette.setColor(QPalette::Foreground, theme.window.text);
        this->setPalette(palette);
    }

    bool BaseWindow::event(QEvent* event)
    {
        if (event->type() ==
            QEvent::WindowDeactivate /*|| event->type() == QEvent::FocusOut*/)
        {
            this->onFocusLost();
        }

        return QWidget::event(event);
    }

    void BaseWindow::wheelEvent(QWheelEvent*)
    {
        // TODO: reimplement
        //    if (event->orientation() != Qt::Vertical)
        //    {
        //        return;
        //    }

        //    if (event->modifiers() & Qt::ControlModifier)
        //    {
        //        if (event->delta() > 0)
        //        {
        //            getSettings()->setClampedUiScale(
        //                getSettings()->getClampedUiScale() + 0.1);
        //        }
        //        else
        //        {
        //            getSettings()->setClampedUiScale(
        //                getSettings()->getClampedUiScale() - 0.1);
        //        }
        //    }
    }

    void BaseWindow::onFocusLost()
    {
        switch (this->getActionOnFocusLoss())
        {
            case Delete:
                this->deleteLater();
                break;

            case Close:
                this->close();
                break;

            case Hide:
                this->hide();
                break;

            default:;
        }
    }

    void BaseWindow::mousePressEvent(QMouseEvent* event)
    {
        // TODO: check on linux/mac
#ifndef Q_OS_WIN
        if (this->flags_.has(FramelessDraggable))
        {
            this->movingRelativePos = event->localPos();
            if (auto widget =
                    this->childAt(event->localPos().x(), event->localPos().y()))
            {
                std::function<bool(QWidget*)> recursiveCheckMouseTracking;
                recursiveCheckMouseTracking = [&](QWidget* widget) {
                    if (widget == nullptr)
                    {
                        return false;
                    }

                    if (widget->hasMouseTracking())
                    {
                        return true;
                    }

                    return recursiveCheckMouseTracking(widget->parentWidget());
                };

                if (!recursiveCheckMouseTracking(widget))
                {
                    // QDebug() << "Start moving";
                    this->moving = true;
                }
            }
        }
#endif

        BaseWidget::mousePressEvent(event);
    }

    void BaseWindow::mouseReleaseEvent(QMouseEvent* event)
    {
#ifndef Q_OS_WIN
        if (this->flags_.has(FramelessDraggable))
        {
            if (this->moving)
            {
                // QDebug() << "Stop moving";
                this->moving = false;
            }
        }
#endif

        BaseWidget::mouseReleaseEvent(event);
    }

    void BaseWindow::mouseMoveEvent(QMouseEvent* event)
    {
#ifndef Q_OS_WIN
        if (this->flags_.has(FramelessDraggable))
        {
            if (this->moving)
            {
                const auto& newPos =
                    event->screenPos() - this->movingRelativePos;
                this->move(newPos.x(), newPos.y());
            }
        }
#endif

        BaseWidget::mouseMoveEvent(event);
    }

    void BaseWindow::changeEvent(QEvent*)
    {
        if (this->hasCustomWindowFrame())
        {
            if (this->ui_.maxButton)
            {
                this->ui_.maxButton->setType(
                    this->windowState() & Qt::WindowMaximized
                        ? TitleBarButtonType::Unmaximize
                        : TitleBarButtonType::Maximize);
            }
        }

#ifndef Q_OS_WIN
        this->update();
#endif
    }

    void BaseWindow::leaveEvent(QEvent*)
    {
    }

    void BaseWindow::moveTo(QWidget* parent, QPoint point, bool offset)
    {
        if (offset)
        {
            point.rx() += 16;
            point.ry() += 16;
        }

        this->move(point);
        this->moveIntoDesktopRect(parent);
    }

    void BaseWindow::resizeEvent(QResizeEvent*)
    {
        this->moveIntoDesktopRect(this);
    }

    void BaseWindow::moveEvent(QMoveEvent* event)
    {
        BaseWidget::moveEvent(event);
    }

    void BaseWindow::closeEvent(QCloseEvent*)
    {
        this->closing();
    }

    void BaseWindow::moveIntoDesktopRect(QWidget* parent)
    {
        if (!this->stayInScreenRect_)
            return;

        // move the widget into the screen geometry if it's not already in there
        QDesktopWidget* desktop = QApplication::desktop();

        QRect s = desktop->availableGeometry(parent);
        QPoint p = this->pos();

        if (p.x() < s.left())
        {
            p.setX(s.left());
        }
        if (p.y() < s.top())
        {
            p.setY(s.top());
        }
        if (p.x() + this->width() > s.right())
        {
            p.setX(s.right() - this->width());
        }
        if (p.y() + this->height() > s.bottom())
        {
            p.setY(s.bottom() - this->height());
        }

        if (p != this->pos())
            this->move(p);
    }

    bool BaseWindow::nativeEvent(
        const QByteArray& eventType, void* message, long* result)
    {
#ifdef USEWINSDK
#    if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
        MSG* msg = *reinterpret_cast<MSG**>(message);
#    else
        MSG* msg = reinterpret_cast<MSG*>(message);
#    endif

        bool returnValue = false;

        switch (msg->message)
        {
                // case WM_LBUTTONDOWN:
                // qDebug() << "a";
                // break;

                // case WM_LBUTTONUP:
                // qDebug() << "b";
                // break;

            case WM_DPICHANGED:
                returnValue = this->handleDPICHANGED(msg);
                break;

            case WM_SHOWWINDOW:
                returnValue = this->handleSHOWWINDOW(msg);
                break;

            case WM_NCCALCSIZE:
                returnValue = this->handleNCCALCSIZE(msg, result);
                break;

            case WM_SIZE:
                returnValue = this->handleSIZE(msg);
                break;

            case WM_NCHITTEST:
                returnValue = this->handleNCHITTEST(msg, result);
                break;

            default:
                return QWidget::nativeEvent(eventType, message, result);
        }

        QWidget::nativeEvent(eventType, message, result);

        return returnValue;
#else
        return QWidget::nativeEvent(eventType, message, result);
#endif
    }

    void BaseWindow::updateScalableQss()
    {
        auto scaled = scaleQss(
            this->scalableQss(), this->nativeScale_, this->nativeScale_);

        this->setStyleSheet(scaled);
        this->removeBackground();

        //    auto scale =
        //        this->nativeScale_ * (this->flags_ & DisableCustomScaling
        //                                  ? 1
        //                                  :
        //                                  getABSettings()->getClampedUiScale());

        //    this->setScale(scale);

        //    for (auto child : this->findChildren<BaseWidget*>())
        //    {
        //        child->setScale(scale);
        //    }

        emit this->scaleChanged();
    }

    void BaseWindow::hideMaximize()
    {
        this->ui_.maxButton->hide();
    }

    // EVENTS

    // WINDOWS
#ifdef USEWINSDK
    bool BaseWindow::handleDPICHANGED(MSG* msg)
    {
        int dpi = HIWORD(msg->wParam);

        float _scale = dpi / 96.f;

        static bool firstResize = false;

        if (!firstResize)
        {
            auto* prcNewWindow = reinterpret_cast<RECT*>(msg->lParam);
            SetWindowPos(msg->hwnd, nullptr, prcNewWindow->left,
                prcNewWindow->top, prcNewWindow->right - prcNewWindow->left,
                prcNewWindow->bottom - prcNewWindow->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        firstResize = false;

        this->nativeScale_ = _scale;
        this->updateScalableQss();

        return true;
    }

    bool BaseWindow::handleSHOWWINDOW(MSG* msg)
    {
        // set initial dpi
        if (auto dpi = getWindowDpi(msg->hwnd))
        {
            this->nativeScale_ = dpi.value() / 96.f;
            this->updateScalableQss();
        }

        // extend window frame
        if (!this->shown_ && this->isVisible() && this->hasCustomWindowFrame())
        {
            this->shown_ = true;

            int cx = GetSystemMetrics(SM_CXSIZEFRAME);
            int cy = GetSystemMetrics(SM_CYSIZEFRAME);

            const MARGINS shadow = {cx, cx, cy, cy};
            DwmExtendFrameIntoClientArea(HWND(this->winId()), &shadow);
        }

        // make topmost
        if (this->flags_.has(TopMost))
        {
            ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        return true;
    }

    bool BaseWindow::handleNCCALCSIZE(MSG* msg, long* result)
    {
        if (this->hasCustomWindowFrame())
        {
            if (msg->wParam == TRUE)
            {
                NCCALCSIZE_PARAMS* ncp =
                    (reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam));
                ncp->lppos->flags |= SWP_NOREDRAW;
                RECT* clientRect = &ncp->rgrc[0];

                clientRect->left += 1;
                clientRect->top += 0;
                clientRect->right -= 1;
                clientRect->bottom -= 1;
            }

            *result = 0;
            return true;
        }
        return false;
    }

    bool BaseWindow::handleSIZE(MSG* msg)
    {
        if (this->ui_.windowLayout)
        {
            if (this->frameless_)
            {
                //
            }
            else if (this->hasCustomWindowFrame())
            {
                if (msg->wParam == SIZE_MAXIMIZED)
                {
                    auto offset = int(this->nativeScale_ * 7);

                    this->ui_.windowLayout->setContentsMargins(
                        offset, offset, offset, offset);
                }
                else
                {
                    this->ui_.windowLayout->setContentsMargins(0, 1, 0, 0);
                }
            }
        }
        return false;
    }

    bool BaseWindow::handleNCHITTEST(MSG* msg, long* result)
    {
        const LONG border_width = 8;  // in pixels
        RECT winrect;
        GetWindowRect(HWND(winId()), &winrect);

        long x = GET_X_LPARAM(msg->lParam);
        long y = GET_Y_LPARAM(msg->lParam);

        QPoint point(x - winrect.left, y - winrect.top);

        if (this->hasCustomWindowFrame())
        {
            *result = 0;

            bool resizeWidth = minimumWidth() != maximumWidth();
            bool resizeHeight = minimumHeight() != maximumHeight();

            if (resizeWidth)
            {
                // left border
                if (x < winrect.left + border_width)
                {
                    *result = HTLEFT;
                }
                // right border
                if (x >= winrect.right - border_width)
                {
                    *result = HTRIGHT;
                }
            }
            if (resizeHeight)
            {
                // bottom border
                if (y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOM;
                }
                // top border
                if (y < winrect.top + border_width)
                {
                    *result = HTTOP;
                }
            }
            if (resizeWidth && resizeHeight)
            {
                // bottom left corner
                if (x >= winrect.left && x < winrect.left + border_width &&
                    y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOMLEFT;
                }
                // bottom right corner
                if (x < winrect.right && x >= winrect.right - border_width &&
                    y < winrect.bottom && y >= winrect.bottom - border_width)
                {
                    *result = HTBOTTOMRIGHT;
                }
                // top left corner
                if (x >= winrect.left && x < winrect.left + border_width &&
                    y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOPLEFT;
                }
                // top right corner
                if (x < winrect.right && x >= winrect.right - border_width &&
                    y >= winrect.top && y < winrect.top + border_width)
                {
                    *result = HTTOPRIGHT;
                }
            }

            if (*result == 0)
            {
                bool client = true;

                if (this->ui_.titleLabel &&
                    this->ui_.titleLabel->geometry().contains(point))
                    client = false;

                if (client)
                {
                    *result = HTCLIENT;
                }
                else
                {
                    *result = HTCAPTION;
                }
            }

            return true;
        }
        else if (this->flags_.has(FramelessDraggable))
        {
            *result = 0;
            bool client = false;

            if (auto widget = this->childAt(point))
            {
                std::function<bool(QWidget*)> recursiveCheckMouseTracking;
                recursiveCheckMouseTracking = [&](QWidget* widget) {
                    if (widget == nullptr)
                    {
                        return false;
                    }

                    if (widget->hasMouseTracking())
                    {
                        return true;
                    }

                    return recursiveCheckMouseTracking(widget->parentWidget());
                };

                if (recursiveCheckMouseTracking(widget))
                {
                    client = true;
                }
            }

            *result = client ? HTCLIENT : HTCAPTION;

            return true;
        }
        return false;
    }
#endif

    // DIALOG
    Dialog::Dialog()
    {
        this->hideMaximize();
    }

    // POPUP
    Popup::Popup()
        : BaseWindow({BaseWindow::Frameless, BaseWindow::TopMost})
    {
        this->setActionOnFocusLoss(ab::BaseWindow::ActionOnFocusLoss::Delete);
    }
}  // namespace ab
