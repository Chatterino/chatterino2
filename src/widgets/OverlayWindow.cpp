#include "widgets/OverlayWindow.hpp"

#include "Application.hpp"
#include "common/FlagsEnum.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PostToThread.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/InvisibleSizeGrip.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/splits/Split.hpp"

#include <QBoxLayout>
#include <QCursor>
#include <QGraphicsEffect>
#include <QGridLayout>
#include <QKeySequence>
#include <QSizeGrip>

#ifdef Q_OS_WIN
#    include <Windows.h>
#    include <windowsx.h>

// This definition can be used to test the move interaction for other platforms
// on Windows by commenting it out. In a final build, Windows must always use
// this, as it's much smoother.
#    define OVERLAY_NATIVE_MOVE
#endif

namespace {

using namespace chatterino;
using namespace literals;

/// Progress the user has made in exploring the overlay
enum class Knowledge : std::int32_t {  // NOLINT(performance-enum-size)
    None = 0,
    // User opened the overlay at least once
    Activation = 1 << 0,
};

bool hasKnowledge(Knowledge knowledge)
{
    FlagsEnum<Knowledge> current(static_cast<Knowledge>(
        getSettings()->overlayKnowledgeLevel.getValue()));
    return current.has(knowledge);
}

void acquireKnowledge(Knowledge knowledge)
{
    FlagsEnum<Knowledge> current(static_cast<Knowledge>(
        getSettings()->overlayKnowledgeLevel.getValue()));
    current.set(knowledge);
    getSettings()->overlayKnowledgeLevel =
        static_cast<std::underlying_type_t<Knowledge>>(current.value());
}

/// Returns [seq?, toggleAllOverlays]
std::pair<QKeySequence, bool> toggleIntertiaShortcut()
{
    auto seq = getApp()->getHotkeys()->getDisplaySequence(
        HotkeyCategory::Split, u"toggleOverlayInertia"_s, {{u"this"_s}});
    if (!seq.isEmpty())
    {
        return {seq, false};
    }
    seq = getApp()->getHotkeys()->getDisplaySequence(
        HotkeyCategory::Split, u"toggleOverlayInertia"_s, {{u"thisOrAll"_s}});
    if (!seq.isEmpty())
    {
        return {seq, false};
    }
    return {
        getApp()->getHotkeys()->getDisplaySequence(HotkeyCategory::Split,
                                                   u"toggleOverlayInertia"_s),
        true,
    };
}

}  // namespace

namespace chatterino {

using namespace std::chrono_literals;

OverlayWindow::OverlayWindow(IndirectChannel channel,
                             const QList<QUuid> &filterIDs)
    : BaseWindow({
          BaseWindow::Frameless,
          BaseWindow::TopMost,
          BaseWindow::DisableLayoutSave,
      })
#ifdef Q_OS_WIN
    , sizeAllCursor_(::LoadCursor(nullptr, IDC_SIZEALL))
#endif
    , channel_(std::move(channel))
    , channelView_(nullptr)
    , interaction_(this)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle(u"Chatterino - Overlay"_s);

    // QGridLayout is (ab)used to stack widgets and position them
    auto *grid = new QGridLayout(this);
    grid->addWidget(&this->channelView_, 0, 0);
    this->interaction_.attach(grid);
#ifndef OVERLAY_NATIVE_MOVE
    grid->addWidget(new InvisibleSizeGrip(this), 0, 0,
                    Qt::AlignBottom | Qt::AlignRight);
#endif

    // the interaction overlay currently captures all events
    this->interaction_.installEventFilter(this);

    this->shortInteraction_.setInterval(750ms);
    QObject::connect(&this->shortInteraction_, &QTimer::timeout, [this] {
        this->endInteraction();
    });

    this->channelView_.installEventFilter(this);
    this->channelView_.setFilters(filterIDs);
    this->channelView_.setChannel(this->channel_.get());
    this->channelView_.setIsOverlay(true);  // use overlay colors
    this->channelView_.setAttribute(Qt::WA_TranslucentBackground);
    this->signalHolder_.managedConnect(
        this->channel_.getChannelChanged(), [this]() {
            this->channelView_.setChannel(this->channel_.get());
        });
    this->channelView_.scrollbar()->setHideThumb(true);
    this->channelView_.scrollbar()->setHideHighlights(true);

    this->setAutoFillBackground(false);
    this->resize(300, 500);
    this->move(QCursor::pos() - this->rect().center());
    this->setContentsMargins(0, 0, 0, 0);
    this->setAttribute(Qt::WA_TranslucentBackground);

    auto *settings = getSettings();
    settings->enableOverlayShadow.connect(
        [this](bool value) {
            if (value)
            {
                this->dropShadow_ = new QGraphicsDropShadowEffect;
                this->channelView_.setGraphicsEffect(this->dropShadow_);
            }
            else
            {
                this->channelView_.setGraphicsEffect(nullptr);
                this->dropShadow_ = nullptr;  // deleted by setGraphicsEffect
            }
            this->applyTheme();
        },
        this->signalHolder_);
    settings->overlayBackgroundOpacity.connect(
        [this] {
            this->channelView_.updateColorTheme();
            this->update();
        },
        this->signalHolder_, false);

    auto applyIt = [this](auto /*unused*/) {
        this->applyTheme();
    };
    settings->overlayShadowOffsetX.connect(applyIt, this->signalHolder_, false);
    settings->overlayShadowOffsetY.connect(applyIt, this->signalHolder_, false);
    settings->overlayShadowOpacity.connect(applyIt, this->signalHolder_, false);
    settings->overlayShadowRadius.connect(applyIt, this->signalHolder_, false);
    settings->overlayShadowColor.connect(applyIt, this->signalHolder_, false);

    this->addShortcuts();
    this->signalHolder_.managedConnect(getApp()->getHotkeys()->onItemsUpdated,
                                       [this]() {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });

    settings->overlayScaleFactor.connect(
        [this] {
            this->updateScale();
        },
        this->signalHolder_, false);
    std::ignore = this->scaleChanged.connect([this](float /*scale*/) {
        this->channelView_.queueLayout();
    });
    this->updateScale();

    this->triggerFirstActivation();
    getApp()->getEmotes()->getGIFTimer().registerOpenOverlayWindow();
}

OverlayWindow::~OverlayWindow()
{
#ifdef Q_OS_WIN
    ::DestroyCursor(this->sizeAllCursor_);
#endif
    getApp()->getEmotes()->getGIFTimer().unregisterOpenOverlayWindow();
}

void OverlayWindow::applyTheme()
{
    auto *settings = getSettings();

    if (this->dropShadow_)
    {
        QColor shadowColor(settings->overlayShadowColor.getValue());
        shadowColor.setAlpha(
            std::clamp(settings->overlayShadowOpacity.getValue(), 0, 255));
        this->dropShadow_->setColor(shadowColor);
        this->dropShadow_->setOffset(settings->overlayShadowOffsetX,
                                     settings->overlayShadowOffsetY);
        this->dropShadow_->setBlurRadius(settings->overlayShadowRadius);
    }
    this->update();
}

float OverlayWindow::desiredScale() const
{
    return getSettings()->getClampedUiScale() *
           getSettings()->getClampedOverlayScale();
}

bool OverlayWindow::eventFilter(QObject * /*object*/, QEvent *event)
{
#ifndef OVERLAY_NATIVE_MOVE
    switch (event->type())
    {
        case QEvent::MouseButtonPress: {
            auto *evt = dynamic_cast<QMouseEvent *>(event);
            this->moving_ = true;
            this->moveOrigin_ = evt->globalPos();
            return true;
        }
        break;
        case QEvent::MouseButtonRelease: {
            if (this->moving_)
            {
                this->moving_ = false;
                return true;
            }
            return false;
        }
        break;
        case QEvent::MouseMove: {
            auto *evt = dynamic_cast<QMouseEvent *>(event);
            if (this->moving_)
            {
                auto newPos = evt->globalPos() - this->moveOrigin_;
                this->move(newPos + this->pos());
                this->moveOrigin_ = evt->globalPos();
                return true;
            }
            if (this->interaction_.isInteracting())
            {
                this->setOverrideCursor(Qt::SizeAllCursor);
                return true;
            }
            return false;
        }
        break;
        default:
            return false;
    }
#else
    (void)event;
    return false;
#endif
}

void OverlayWindow::setOverrideCursor(const QCursor &cursor)
{
    this->channelView_.setCursor(cursor);
    this->setCursor(cursor);
}

bool OverlayWindow::isInert() const
{
    return this->inert_;
}

void OverlayWindow::toggleInertia()
{
    this->setInert(!this->inert_);
}

void OverlayWindow::enterEvent(EnterEvent * /*event*/)
{
#ifndef OVERLAY_NATIVE_MOVE
    this->startInteraction();
#endif
}

void OverlayWindow::leaveEvent(QEvent * /*event*/)
{
#ifndef OVERLAY_NATIVE_MOVE
    this->endInteraction();
#endif
}

#ifdef Q_OS_WIN
bool OverlayWindow::nativeEvent(const QByteArray &eventType, void *message,
                                NativeResult *result)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    MSG *msg = reinterpret_cast<MSG *>(message);

    bool returnValue = false;

    switch (msg->message)
    {
#    ifdef OVERLAY_NATIVE_MOVE
        case WM_NCHITTEST:
            this->handleNCHITTEST(msg, result);
            returnValue = true;
            break;
        case WM_MOUSEMOVE:
        case WM_NCMOUSEMOVE:
            this->startShortInteraction();
            break;
        case WM_ENTERSIZEMOVE:
            this->startInteraction();
            break;
        case WM_EXITSIZEMOVE:
            // wait a few seconds before hiding
            this->startShortInteraction();
            break;
        case WM_SETCURSOR: {
            // When the window can be moved, the size-all cursor should be
            // shown. Qt doesn't provide an interface to do this, so this
            // manually sets the cursor.
            if (LOWORD(msg->lParam) == HTCAPTION)
            {
                ::SetCursor(this->sizeAllCursor_);
                *result = TRUE;
                returnValue = true;
            }
        }
        break;
#    endif
        case WM_DPICHANGED: {
            // wait for Qt to process this message, same as in BaseWindow
            postToThread([] {
                getApp()->getWindows()->invalidateChannelViewBuffers();
            });
        }
        break;

        default:
            return BaseWindow::nativeEvent(eventType, message, result);
    }

    return returnValue;
}

void OverlayWindow::handleNCHITTEST(MSG *msg, NativeResult *result)
{
    // This implementation is similar to the one of BaseWindow, but has the
    // following differences:
    // - The window can always be resized (or: it can't be maximized)
    // - The close button is advertised as HTCLIENT instead of HTCLOSE
    // - There isn't any other client area (the entire window can be moved)
    const LONG borderWidth = 8;  // in device independent pixels

    auto rect = this->rect();

    POINT p{GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
    ScreenToClient(msg->hwnd, &p);

    QPoint point(p.x, p.y);
    point /= this->devicePixelRatio();

    auto x = point.x();
    auto y = point.y();

    *result = 0;

    // left border
    if (x < rect.left() + borderWidth)
    {
        *result = HTLEFT;
    }
    // right border
    if (x >= rect.right() - borderWidth)
    {
        *result = HTRIGHT;
    }

    // bottom border
    if (y >= rect.bottom() - borderWidth)
    {
        *result = HTBOTTOM;
    }
    // top border
    if (y < rect.top() + borderWidth)
    {
        *result = HTTOP;
    }

    // bottom left corner
    if (x >= rect.left() && x < rect.left() + borderWidth &&
        y < rect.bottom() && y >= rect.bottom() - borderWidth)
    {
        *result = HTBOTTOMLEFT;
    }
    // bottom right corner
    if (x < rect.right() && x >= rect.right() - borderWidth &&
        y < rect.bottom() && y >= rect.bottom() - borderWidth)
    {
        *result = HTBOTTOMRIGHT;
    }
    // top left corner
    if (x >= rect.left() && x < rect.left() + borderWidth && y >= rect.top() &&
        y < rect.top() + borderWidth)
    {
        *result = HTTOPLEFT;
    }
    // top right corner
    if (x < rect.right() && x >= rect.right() - borderWidth &&
        y >= rect.top() && y < rect.top() + borderWidth)
    {
        *result = HTTOPRIGHT;
    }

    if (*result == 0)
    {
        auto *closeButton = this->interaction_.closeButton();
        if (closeButton->isVisible() && closeButton->geometry().contains(point))
        {
            *result = HTCLIENT;
        }
        else
        {
            *result = HTCAPTION;
        }
    }
}
#endif

void OverlayWindow::triggerFirstActivation()
{
    if (hasKnowledge(Knowledge::Activation))
    {
        return;
    }
    acquireKnowledge(Knowledge::Activation);

    auto welcomeText =
        u"Hey! It looks like this is the first time you're using the overlay. "_s
        "You can move the overlay by dragging it with your mouse. "
#ifdef OVERLAY_NATIVE_MOVE
        "To resize the window, drag on any edge."
#else
        "To resize the window, drag on the bottom right corner."
#endif
        "<br><br>"
        "By default, the overlay is interactive. ";

    auto [actualShortcut, allOverlays] = toggleIntertiaShortcut();
    if (actualShortcut.isEmpty())
    {
        welcomeText +=
            u"To toggle the click-through mode, "
            "add a hotkey for \"Toggle overlay click-through\" in the split "
            "category to press while any Chatterino window is focused."_s;
    }
    else
    {
        welcomeText +=
            u"To toggle the click-through mode, press %1 (customizable "_s
            "in the settings) while any Chatterino window is focused.".arg(
                actualShortcut.toString());
    }

    welcomeText += u"<br><br>"_s
                   "This is still an early version and some features are "
                   "missing. Please provide feedback <a "
                   "href=\"https://github.com/Chatterino/chatterino2/"
                   "discussions\">on GitHub</a>.";

    auto *box =
        new QMessageBox(QMessageBox::Information, u"Chatterino - Overlay"_s,
                        welcomeText, QMessageBox::Ok, this);
    box->open();
}

void OverlayWindow::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"zoom",
         [](const std::vector<QString> &arguments) -> QString {
             if (arguments.size() == 0)
             {
                 qCWarning(chatterinoHotkeys)
                     << "zoom shortcut called without arguments. Takes "
                        "only "
                        "one argument: \"in\", \"out\", or \"reset\"";
                 return "zoom shortcut called without arguments. Takes "
                        "only "
                        "one argument: \"in\", \"out\", or \"reset\"";
             }
             auto change = 0.0F;
             const auto &direction = arguments.at(0);
             if (direction == "reset")
             {
                 getSettings()->uiScale.setValue(1);
                 return "";
             }

             if (direction == u"in")
             {
                 change = 0.1F;
             }
             else if (direction == u"out")
             {
                 change = -0.1F;
             }
             else
             {
                 qCWarning(chatterinoHotkeys)
                     << "Invalid zoom direction, use \"in\", \"out\", or "
                        "\"reset\"";
                 return "Invalid zoom direction, use \"in\", \"out\", or "
                        "\"reset\"";
             }
             getSettings()->setClampedOverlayScale(
                 getSettings()->getClampedOverlayScale() + change);
             return {};
         }},
    };

    this->shortcuts_ = getApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::Window, actions, this);

    auto [seq, allOverlays] = toggleIntertiaShortcut();
    if (!seq.isEmpty())
    {
        auto *inertiaShortcut = new QShortcut(seq, this);
        if (allOverlays)
        {
            QObject::connect(inertiaShortcut, &QShortcut::activated, this, [] {
                getApp()->getWindows()->toggleAllOverlayInertia();
            });
        }
        else
        {
            QObject::connect(inertiaShortcut, &QShortcut::activated, this,
                             &OverlayWindow::toggleInertia);
        }
        this->shortcuts_.push_back(inertiaShortcut);
    }
}

void OverlayWindow::startInteraction()
{
    if (this->inert_)
    {
        return;
    }

    this->interaction_.startInteraction();
    this->shortInteraction_.stop();
}

void OverlayWindow::startShortInteraction()
{
    if (this->inert_)
    {
        return;
    }

    this->interaction_.startInteraction();
    this->shortInteraction_.start();
}

void OverlayWindow::endInteraction()
{
    this->interaction_.endInteraction();
}

void OverlayWindow::setInert(bool inert)
{
    if (this->inert_ == inert)
    {
        return;
    }

    this->inert_ = inert;

    this->setWindowFlag(Qt::WindowTransparentForInput, inert);
    if (this->isHidden())
    {
        this->show();
    }
    this->endInteraction();

    if (inert)
    {
        if (this->channelView_.scrollbar()->isVisible())
        {
            this->channelView_.scrollbar()->scrollToBottom();
        }
        this->interaction_.hide();
    }
    else
    {
        this->interaction_.show();
    }
}

}  // namespace chatterino
