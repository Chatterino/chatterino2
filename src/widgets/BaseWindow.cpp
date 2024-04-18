#include "widgets/BaseWindow.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/DebugCount.hpp"
#include "util/PostToThread.hpp"
#include "util/WindowsHelper.hpp"
#include "widgets/helper/EffectLabel.hpp"
#include "widgets/helper/TitlebarButtons.hpp"
#include "widgets/Label.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QScreen>

#include <functional>

#ifdef USEWINSDK
#    include <dwmapi.h>
#    include <VersionHelpers.h>
#    include <Windows.h>
#    include <windowsx.h>

#    pragma comment(lib, "Dwmapi.lib")

#    include <QHBoxLayout>

#    define WM_DPICHANGED 0x02E0
#endif

#include "widgets/helper/TitlebarButton.hpp"

namespace chatterino {

BaseWindow::BaseWindow(FlagsEnum<Flags> _flags, QWidget *parent)
    : BaseWidget(parent, (_flags.has(Dialog) ? Qt::Dialog : Qt::Window) |
                             (_flags.has(TopMost) ? Qt::WindowStaysOnTopHint
                                                  : Qt::WindowFlags()))
    , enableCustomFrame_(_flags.has(EnableCustomFrame))
    , frameless_(_flags.has(Frameless))
    , flags_(_flags)
{
    if (this->frameless_)
    {
        this->enableCustomFrame_ = false;
        this->setWindowFlag(Qt::FramelessWindowHint);
    }

    if (_flags.has(DontFocus))
    {
        this->setAttribute(Qt::WA_ShowWithoutActivating);
#ifdef Q_OS_LINUX
        this->setWindowFlags(Qt::ToolTip);
#else
        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                             Qt::WindowDoesNotAcceptFocus |
                             Qt::BypassWindowManagerHint);
#endif
    }

    this->init();

    getSettings()->uiScale.connect(
        [this]() {
            postToThread([this] {
                this->updateScale();
                this->updateScale();
            });
        },
        this->connections_, false);

    this->updateScale();

    this->resize(300, 150);

#ifdef USEWINSDK
    this->useNextBounds_.setSingleShot(true);
    QObject::connect(&this->useNextBounds_, &QTimer::timeout, this, [this]() {
        this->currentBounds_ = this->nextBounds_;
    });
#endif

    this->themeChangedEvent();
    DebugCount::increase("BaseWindow");
}

BaseWindow::~BaseWindow()
{
    DebugCount::decrease("BaseWindow");
}

void BaseWindow::setInitialBounds(const QRect &bounds)
{
#ifdef USEWINSDK
    this->initalBounds_ = bounds;
#else
    this->setGeometry(bounds);
#endif
}

QRect BaseWindow::getBounds()
{
#ifdef USEWINSDK
    return this->currentBounds_;
#else
    return this->geometry();
#endif
}

float BaseWindow::scale() const
{
    return std::max<float>(0.01f, this->overrideScale().value_or(this->scale_));
}

float BaseWindow::qtFontScale() const
{
    return this->scale() / std::max<float>(0.01F, this->nativeScale_);
}

void BaseWindow::init()
{
#ifdef USEWINSDK
    if (this->hasCustomWindowFrame())
    {
        // CUSTOM WINDOW FRAME
        QVBoxLayout *layout = new QVBoxLayout();
        this->ui_.windowLayout = layout;
        layout->setContentsMargins(1, 1, 1, 1);
        layout->setSpacing(0);
        this->setLayout(layout);
        {
            if (!this->frameless_)
            {
                QHBoxLayout *buttonLayout = this->ui_.titlebarBox =
                    new QHBoxLayout();
                buttonLayout->setContentsMargins(0, 0, 0, 0);
                layout->addLayout(buttonLayout);

                // title
                Label *title = new Label;
                QObject::connect(this, &QWidget::windowTitleChanged,
                                 [title](const QString &text) {
                                     title->setText(text);
                                 });

                QSizePolicy policy(QSizePolicy::Ignored,
                                   QSizePolicy::Preferred);
                policy.setHorizontalStretch(1);
                title->setSizePolicy(policy);
                buttonLayout->addWidget(title);
                this->ui_.titleLabel = title;

                // buttons
                TitleBarButton *_minButton = new TitleBarButton;
                _minButton->setButtonStyle(TitleBarButtonStyle::Minimize);
                TitleBarButton *_maxButton = new TitleBarButton;
                _maxButton->setButtonStyle(TitleBarButtonStyle::Maximize);
                TitleBarButton *_exitButton = new TitleBarButton;
                _exitButton->setButtonStyle(TitleBarButtonStyle::Close);

                QObject::connect(_minButton, &TitleBarButton::leftClicked, this,
                                 [this] {
                                     this->setWindowState(Qt::WindowMinimized |
                                                          this->windowState());
                                 });
                QObject::connect(_maxButton, &TitleBarButton::leftClicked, this,
                                 [this, _maxButton] {
                                     this->setWindowState(
                                         _maxButton->getButtonStyle() !=
                                                 TitleBarButtonStyle::Maximize
                                             ? Qt::WindowActive
                                             : Qt::WindowMaximized);
                                 });
                QObject::connect(_exitButton, &TitleBarButton::leftClicked,
                                 this, [this] {
                                     this->close();
                                 });

                this->ui_.titlebarButtons = new TitleBarButtons(
                    this, _minButton, _maxButton, _exitButton);

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
        this->ui_.layoutBase->setContentsMargins(1, 0, 1, 1);
        layout->addWidget(this->ui_.layoutBase);
    }

// DPI
//    auto dpi = getWindowDpi(this->safeHWND());

//    if (dpi) {
//        this->scale = dpi.value() / 96.f;
//    }
#endif

    // TopMost flag overrides setting
    if (!this->flags_.has(TopMost))
    {
        getSettings()->windowTopMost.connect(
            [this](bool topMost) {
                this->setTopMost(topMost);
            },
            this->connections_);
    }
}

void BaseWindow::setTopMost(bool topMost)
{
    if (this->flags_.has(TopMost))
    {
        qCWarning(chatterinoWidget)
            << "Called setTopMost on a window with the `TopMost` flag set.";
        return;
    }

    if (this->isTopMost_ == topMost)
    {
        return;
    }
    this->isTopMost_ = topMost;

#ifdef USEWINSDK
    if (!this->waitingForTopMost_)
    {
        this->tryApplyTopMost();
    }
#else
    auto isVisible = this->isVisible();
    this->setWindowFlag(Qt::WindowStaysOnTopHint, topMost);
    if (isVisible)
    {
        this->show();
    }
#endif

    this->topMostChanged(this->isTopMost_);
}

#ifdef USEWINSDK
void BaseWindow::tryApplyTopMost()
{
    auto hwnd = this->safeHWND();
    if (!hwnd)
    {
        this->waitingForTopMost_ = true;
        QTimer::singleShot(50, this, &BaseWindow::tryApplyTopMost);
        return;
    }
    this->waitingForTopMost_ = false;

    if (this->parent())
    {
        // Don't change the topmost value of child windows. This would apply
        // to the top-level window too.
        return;
    }

    ::SetWindowPos(*hwnd, this->isTopMost_ ? HWND_TOPMOST : HWND_NOTOPMOST, 0,
                   0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
#endif

bool BaseWindow::isTopMost() const
{
    return this->isTopMost_ || this->flags_.has(TopMost);
}

void BaseWindow::setActionOnFocusLoss(ActionOnFocusLoss value)
{
    this->actionOnFocusLoss_ = value;
}

BaseWindow::ActionOnFocusLoss BaseWindow::getActionOnFocusLoss() const
{
    return this->actionOnFocusLoss_;
}

QWidget *BaseWindow::getLayoutContainer()
{
    if (this->hasCustomWindowFrame())
    {
        return this->ui_.layoutBase;
    }
    else
    {
        return this;
    }
}

bool BaseWindow::hasCustomWindowFrame()
{
    return BaseWindow::supportsCustomWindowFrame() && this->enableCustomFrame_;
}

bool BaseWindow::supportsCustomWindowFrame()
{
#ifdef USEWINSDK
    static bool isWin8 = IsWindows8OrGreater();

    return isWin8;
#else
    return false;
#endif
}

void BaseWindow::themeChangedEvent()
{
    if (this->hasCustomWindowFrame())
    {
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(80, 80, 80, 255));
        palette.setColor(QPalette::WindowText, this->theme->window.text);
        this->setPalette(palette);

        if (this->ui_.titleLabel)
        {
            QPalette palette_title;
            palette_title.setColor(
                QPalette::WindowText,
                this->theme->isLightTheme() ? "#333" : "#ccc");
            this->ui_.titleLabel->setPalette(palette_title);
        }

        for (Button *button : this->ui_.buttons)
        {
            button->setMouseEffectColor(this->theme->window.text);
        }
    }
    else
    {
        QPalette palette;
        palette.setColor(QPalette::Window, this->theme->window.background);
        palette.setColor(QPalette::WindowText, this->theme->window.text);
        this->setPalette(palette);
    }
}

bool BaseWindow::event(QEvent *event)
{
    if (event->type() ==
        QEvent::WindowDeactivate /*|| event->type() == QEvent::FocusOut*/)
    {
        this->onFocusLost();
    }

    return QWidget::event(event);
}

void BaseWindow::wheelEvent(QWheelEvent *event)
{
    // ignore horizontal mouse wheels
    if (event->angleDelta().x() != 0)
    {
        return;
    }

    if (event->modifiers() & Qt::ControlModifier)
    {
        if (event->angleDelta().y() > 0)
        {
            getSettings()->setClampedUiScale(
                getSettings()->getClampedUiScale() + 0.1);
        }
        else
        {
            getSettings()->setClampedUiScale(
                getSettings()->getClampedUiScale() - 0.1);
        }
    }
}

void BaseWindow::onFocusLost()
{
    switch (this->getActionOnFocusLoss())
    {
        case Delete: {
            this->deleteLater();
        }
        break;

        case Close: {
            this->close();
        }
        break;

        case Hide: {
            this->hide();
        }
        break;

        default:;
    }
}

void BaseWindow::mousePressEvent(QMouseEvent *event)
{
#ifndef Q_OS_WIN
    if (this->flags_.has(FramelessDraggable))
    {
        this->movingRelativePos = event->localPos();
        if (auto *widget =
                this->childAt(event->localPos().x(), event->localPos().y()))
        {
            std::function<bool(QWidget *)> recursiveCheckMouseTracking;
            recursiveCheckMouseTracking = [&](QWidget *widget) {
                if (widget == nullptr || widget->isHidden())
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
                this->moving = true;
            }
        }
    }
#endif

    BaseWidget::mousePressEvent(event);
}

void BaseWindow::mouseReleaseEvent(QMouseEvent *event)
{
#ifndef Q_OS_WIN
    if (this->flags_.has(FramelessDraggable))
    {
        if (this->moving)
        {
            this->moving = false;
        }
    }
#endif

    BaseWidget::mouseReleaseEvent(event);
}

void BaseWindow::mouseMoveEvent(QMouseEvent *event)
{
#ifndef Q_OS_WIN
    if (this->flags_.has(FramelessDraggable))
    {
        if (this->moving)
        {
            const auto &newPos = event->screenPos() - this->movingRelativePos;
            this->move(newPos.x(), newPos.y());
        }
    }
#endif

    BaseWidget::mouseMoveEvent(event);
}

TitleBarButton *BaseWindow::addTitleBarButton(const TitleBarButtonStyle &style,
                                              std::function<void()> onClicked)
{
    TitleBarButton *button = new TitleBarButton;
    button->setScaleIndependantSize(30, 30);

    this->ui_.buttons.push_back(button);
    this->ui_.titlebarBox->insertWidget(1, button);
    button->setButtonStyle(style);

    QObject::connect(button, &TitleBarButton::leftClicked, this, [onClicked] {
        onClicked();
    });

    return button;
}

EffectLabel *BaseWindow::addTitleBarLabel(std::function<void()> onClicked)
{
    EffectLabel *button = new EffectLabel;
    button->setScaleIndependantHeight(30);

    this->ui_.buttons.push_back(button);
    this->ui_.titlebarBox->insertWidget(1, button);

    QObject::connect(button, &EffectLabel::leftClicked, this, [onClicked] {
        onClicked();
    });

    return button;
}

void BaseWindow::changeEvent(QEvent *)
{
#ifdef USEWINSDK
    if (this->ui_.titlebarButtons)
    {
        this->ui_.titlebarButtons->updateMaxButton();
    }

    if (this->isVisible() && this->hasCustomWindowFrame())
    {
        auto hwnd = this->safeHWND();
        if (hwnd)
        {
            auto palette = this->palette();
            palette.setColor(QPalette::Window, GetForegroundWindow() == *hwnd
                                                   ? QColor(90, 90, 90)
                                                   : QColor(50, 50, 50));
            this->setPalette(palette);
        }
    }
#endif

#ifndef Q_OS_WIN
    this->update();
#endif
}

void BaseWindow::leaveEvent(QEvent *)
{
    this->leaving.invoke();
}

void BaseWindow::moveTo(QPoint point, widgets::BoundsChecking mode)
{
    this->lastBoundsCheckPosition_ = point;
    this->lastBoundsCheckMode_ = mode;
    widgets::moveWindowTo(this, point, mode);
}

void BaseWindow::showAndMoveTo(QPoint point, widgets::BoundsChecking mode)
{
    this->lastBoundsCheckPosition_ = point;
    this->lastBoundsCheckMode_ = mode;
    widgets::showAndMoveWindowTo(this, point, mode);
}

bool BaseWindow::applyLastBoundsCheck()
{
    if (this->lastBoundsCheckMode_ == widgets::BoundsChecking::Off)
    {
        return false;
    }

    this->moveTo(this->lastBoundsCheckPosition_, this->lastBoundsCheckMode_);
    return true;
}

void BaseWindow::resizeEvent(QResizeEvent *)
{
    // Queue up save because: Window resized
    if (!flags_.has(DisableLayoutSave))
    {
        getIApp()->getWindows()->queueSave();
    }

#ifdef USEWINSDK
    if (this->hasCustomWindowFrame() && !this->isResizeFixing_)
    {
        this->isResizeFixing_ = true;
        QTimer::singleShot(50, this, [this] {
            auto hwnd = this->safeHWND();
            if (!hwnd)
            {
                this->isResizeFixing_ = false;
                return;
            }
            RECT rect;
            ::GetWindowRect(*hwnd, &rect);
            ::SetWindowPos(*hwnd, nullptr, 0, 0, rect.right - rect.left + 1,
                           rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
            ::SetWindowPos(*hwnd, nullptr, 0, 0, rect.right - rect.left,
                           rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);
            QTimer::singleShot(10, this, [this] {
                this->isResizeFixing_ = false;
            });
        });
    }

    this->calcButtonsSizes();
#endif
}

void BaseWindow::moveEvent(QMoveEvent *event)
{
    // Queue up save because: Window position changed
#ifdef CHATTERINO
    if (!flags_.has(DisableLayoutSave))
    {
        getIApp()->getWindows()->queueSave();
    }
#endif

    BaseWidget::moveEvent(event);
}

void BaseWindow::closeEvent(QCloseEvent *)
{
    this->closing.invoke();
}

void BaseWindow::showEvent(QShowEvent *)
{
#ifdef Q_OS_WIN
    if (this->flags_.has(BoundsCheckOnShow))
    {
        this->moveTo(this->pos(), widgets::BoundsChecking::CursorPosition);
    }

    if (!this->flags_.has(TopMost))
    {
        QTimer::singleShot(1, this, [this] {
            if (!this->waitingForTopMost_)
            {
                this->tryApplyTopMost();
            }
        });
    }
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool BaseWindow::nativeEvent(const QByteArray &eventType, void *message,
                             qintptr *result)
#else
bool BaseWindow::nativeEvent(const QByteArray &eventType, void *message,
                             long *result)
#endif
{
#ifdef USEWINSDK
    MSG *msg = reinterpret_cast<MSG *>(message);

    bool returnValue = false;

    auto isHoveringTitlebarButton = [&]() {
        auto ht = msg->wParam;
        return ht == HTMAXBUTTON || ht == HTMINBUTTON || ht == HTCLOSE;
    };

    switch (msg->message)
    {
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

        case WM_MOVE:
            returnValue = this->handleMOVE(msg);
            *result = 0;
            break;

        case WM_NCHITTEST:
            returnValue = this->handleNCHITTEST(msg, result);
            break;

        case WM_NCMOUSEHOVER:
        case WM_NCMOUSEMOVE: {
            // WM_NCMOUSEMOVE/WM_NCMOUSEHOVER gets sent when the mouse is
            // moving/hovering in the non-client area
            // - (mostly) the edges and the titlebar.
            // We only need to handle the event for the titlebar buttons,
            // as Qt doesn't create mouse events for these events.
            if (!this->ui_.titlebarButtons)
            {
                // we don't consume the event if we don't have custom buttons
                break;
            }

            if (isHoveringTitlebarButton())
            {
                *result = 0;
                returnValue = true;
                long x = GET_X_LPARAM(msg->lParam);
                long y = GET_Y_LPARAM(msg->lParam);

                RECT winrect;
                GetWindowRect(msg->hwnd, &winrect);
                QPoint globalPos(x, y);
                this->ui_.titlebarButtons->hover(msg->wParam, globalPos);
                this->lastEventWasNcMouseMove_ = true;
            }
            else
            {
                this->ui_.titlebarButtons->leave();
            }
        }
        break;

        case WM_MOUSEMOVE: {
            if (!this->lastEventWasNcMouseMove_)
            {
                break;
            }
            this->lastEventWasNcMouseMove_ = false;
            // Windows doesn't send WM_NCMOUSELEAVE in some cases,
            // so the buttons show as hovered even though they're not hovered.
            [[fallthrough]];
        }
        case WM_NCMOUSELEAVE: {
            // WM_NCMOUSELEAVE gets sent when the mouse leaves any
            // non-client area. In case we have titlebar buttons,
            // we want to ensure they're deselected.
            if (this->ui_.titlebarButtons)
            {
                this->ui_.titlebarButtons->leave();
            }
        }
        break;

        case WM_NCLBUTTONDOWN:
        case WM_NCLBUTTONUP: {
            // WM_NCLBUTTON{DOWN, UP} gets called when the left mouse button
            // was pressed in a non-client area.
            // We simulate a mouse down/up event for the titlebar buttons
            // as Qt doesn't create an event in that case.
            if (!this->ui_.titlebarButtons || !isHoveringTitlebarButton())
            {
                break;
            }
            returnValue = true;
            *result = 0;

            auto ht = msg->wParam;
            long x = GET_X_LPARAM(msg->lParam);
            long y = GET_Y_LPARAM(msg->lParam);

            RECT winrect;
            GetWindowRect(msg->hwnd, &winrect);
            QPoint globalPos(x, y);
            if (msg->message == WM_NCLBUTTONDOWN)
            {
                this->ui_.titlebarButtons->mousePress(ht, globalPos);
            }
            else
            {
                this->ui_.titlebarButtons->mouseRelease(ht, globalPos);
            }
        }
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

void BaseWindow::scaleChangedEvent(float scale)
{
#ifdef USEWINSDK
    this->calcButtonsSizes();
#endif

    this->setFont(
        getIApp()->getFonts()->getFont(FontStyle::UiTabs, this->qtFontScale()));
}

void BaseWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (this->frameless_)
    {
        painter.setPen(QColor("#999"));
        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }

    this->drawCustomWindowFrame(painter);
}

void BaseWindow::updateScale()
{
    auto scale =
        this->nativeScale_ * (this->flags_.has(DisableCustomScaling)
                                  ? 1
                                  : getSettings()->getClampedUiScale());

    this->setScale(scale);

    for (auto *child : this->findChildren<BaseWidget *>())
    {
        child->setScale(scale);
    }
}

void BaseWindow::calcButtonsSizes()
{
    if (!this->shown_)
    {
        return;
    }

    if (this->frameless_ || !this->ui_.titlebarButtons)
    {
        return;
    }

#ifdef USEWINSDK
    if ((static_cast<float>(this->width()) / this->scale()) < 300)
    {
        this->ui_.titlebarButtons->setSmallSize();
    }
    else
    {
        this->ui_.titlebarButtons->setRegularSize();
    }
#endif
}

void BaseWindow::drawCustomWindowFrame(QPainter &painter)
{
#ifdef USEWINSDK
    if (this->hasCustomWindowFrame())
    {
        QColor bg = this->overrideBackgroundColor_.value_or(
            this->theme->window.background);
        painter.fillRect(QRect(1, 2, this->width() - 2, this->height() - 3),
                         bg);
    }
#endif
}

bool BaseWindow::handleDPICHANGED(MSG *msg)
{
#ifdef USEWINSDK
    int dpi = HIWORD(msg->wParam);

    float _scale = dpi / 96.f;

    auto *prcNewWindow = reinterpret_cast<RECT *>(msg->lParam);
    SetWindowPos(msg->hwnd, nullptr, prcNewWindow->left, prcNewWindow->top,
                 prcNewWindow->right - prcNewWindow->left,
                 prcNewWindow->bottom - prcNewWindow->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);

    this->nativeScale_ = _scale;
    this->updateScale();

    return true;
#else
    return false;
#endif
}

bool BaseWindow::handleSHOWWINDOW(MSG *msg)
{
#ifdef USEWINSDK
    // ignore window hide event
    if (!msg->wParam)
    {
        return true;
    }

    if (auto dpi = getWindowDpi(msg->hwnd))
    {
        float currentScale = (float)dpi.value() / 96.F;
        if (currentScale != this->nativeScale_)
        {
            this->nativeScale_ = currentScale;
            this->updateScale();
        }
    }

    if (!this->shown_)
    {
        this->shown_ = true;

        if (this->hasCustomWindowFrame())
        {
            // disable OS window border
            const MARGINS margins = {-1};
            DwmExtendFrameIntoClientArea(msg->hwnd, &margins);
        }

        if (!this->initalBounds_.isNull())
        {
            ::SetWindowPos(msg->hwnd, nullptr, this->initalBounds_.x(),
                           this->initalBounds_.y(), this->initalBounds_.width(),
                           this->initalBounds_.height(),
                           SWP_NOZORDER | SWP_NOACTIVATE);
            this->currentBounds_ = this->initalBounds_;
        }

        this->calcButtonsSizes();
    }

    return true;
#else
    return false;
#endif
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool BaseWindow::handleNCCALCSIZE(MSG *msg, qintptr *result)
#else
bool BaseWindow::handleNCCALCSIZE(MSG *msg, long *result)
#endif
{
#ifdef USEWINSDK
    if (this->hasCustomWindowFrame())
    {
        if (msg->wParam == TRUE)
        {
            // remove 1 extra pixel on top of custom frame
            auto *ncp = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            if (ncp)
            {
                ncp->lppos->flags |= SWP_NOREDRAW;
                ncp->rgrc[0].top -= 1;
            }
        }

        *result = 0;
        return true;
    }
    return false;
#else
    return false;
#endif
}

bool BaseWindow::handleSIZE(MSG *msg)
{
#ifdef USEWINSDK
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
                auto offset =
                    int(getWindowDpi(msg->hwnd).value_or(96) * 8 / 96);

                this->ui_.windowLayout->setContentsMargins(offset, offset,
                                                           offset, offset);
            }
            else
            {
                this->ui_.windowLayout->setContentsMargins(0, 1, 0, 0);
            }

            this->isNotMinimizedOrMaximized_ = msg->wParam == SIZE_RESTORED;

            if (this->isNotMinimizedOrMaximized_)
            {
                RECT rect;
                ::GetWindowRect(msg->hwnd, &rect);
                this->currentBounds_ =
                    QRect(QPoint(rect.left, rect.top),
                          QPoint(rect.right - 1, rect.bottom - 1));
            }
            this->useNextBounds_.stop();

            if (msg->wParam == SIZE_MINIMIZED && this->ui_.titlebarButtons)
            {
                // Windows doesn't send a WM_NCMOUSELEAVE event when clicking
                // the minimize button, so we have to emulate it.
                this->ui_.titlebarButtons->leave();
            }
        }
    }
    return false;
#else
    return false;
#endif
}

bool BaseWindow::handleMOVE(MSG *msg)
{
#ifdef USEWINSDK
    if (this->isNotMinimizedOrMaximized_)
    {
        RECT rect;
        ::GetWindowRect(msg->hwnd, &rect);
        this->nextBounds_ = QRect(QPoint(rect.left, rect.top),
                                  QPoint(rect.right - 1, rect.bottom - 1));

        this->useNextBounds_.start(10);
    }
#endif
    return false;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool BaseWindow::handleNCHITTEST(MSG *msg, qintptr *result)
#else
bool BaseWindow::handleNCHITTEST(MSG *msg, long *result)
#endif
{
#ifdef USEWINSDK
    const LONG border_width = 8;  // in pixels
    RECT winrect;
    GetWindowRect(msg->hwnd, &winrect);

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
            // Check the main layout first, as it's the largest area
            if (this->ui_.layoutBase->geometry().contains(point))
            {
                *result = HTCLIENT;
            }

            // Check the titlebar buttons
            if (*result == 0 &&
                this->ui_.titlebarBox->geometry().contains(point))
            {
                for (const auto *widget : this->ui_.buttons)
                {
                    if (!widget->isVisible() ||
                        !widget->geometry().contains(point))
                    {
                        continue;
                    }

                    if (const auto *btn =
                            dynamic_cast<const TitleBarButton *>(widget))
                    {
                        switch (btn->getButtonStyle())
                        {
                            case TitleBarButtonStyle::Minimize: {
                                *result = HTMINBUTTON;
                                break;
                            }
                            case TitleBarButtonStyle::Unmaximize:
                            case TitleBarButtonStyle::Maximize: {
                                *result = HTMAXBUTTON;
                                break;
                            }
                            case TitleBarButtonStyle::Close: {
                                *result = HTCLOSE;
                                break;
                            }
                            default: {
                                *result = HTCLIENT;
                                break;
                            }
                        }
                        break;
                    }
                    *result = HTCLIENT;
                    break;
                }
            }

            if (*result == 0)
            {
                *result = HTCAPTION;
            }
        }

        return true;
    }

    if (this->flags_.has(FramelessDraggable))
    {
        *result = 0;
        bool client = false;

        if (auto *widget = this->childAt(point))
        {
            std::function<bool(QWidget *)> recursiveCheckMouseTracking;
            recursiveCheckMouseTracking = [&](QWidget *widget) {
                if (widget == nullptr || widget->isHidden())
                {
                    return false;
                }

                if (widget->hasMouseTracking())
                {
                    return true;
                }

                if (widget == this)
                {
                    return false;
                }

                return recursiveCheckMouseTracking(widget->parentWidget());
            };

            if (recursiveCheckMouseTracking(widget))
            {
                client = true;
            }
        }

        if (client)
        {
            *result = HTCLIENT;
        }
        else
        {
            *result = HTCAPTION;
        }

        return true;
    }

    // don't handle the message
    return false;
#else
    return false;
#endif
}

#ifdef USEWINSDK
std::optional<HWND> BaseWindow::safeHWND() const
{
    if (!this->testAttribute(Qt::WA_WState_Created))
    {
        return std::nullopt;
    }
    return reinterpret_cast<HWND>(this->winId());
}
#endif

}  // namespace chatterino
