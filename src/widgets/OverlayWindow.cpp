#include "widgets/OverlayWindow.hpp"

#include "common/Literals.hpp"
#include "controllers/hotkeys/GlobalShortcut.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/InvisibleSizeGrip.hpp"
#include "widgets/helper/TitlebarButton.hpp"
#include "widgets/Scrollbar.hpp"
#include "widgets/splits/Split.hpp"

#include <QBoxLayout>
#include <QCursor>
#include <QGraphicsEffect>
#include <QGridLayout>
#include <QKeySequence>
#include <QSizeGrip>

#include <array>

namespace {

using namespace chatterino;
using namespace literals;

enum class Knowledge : std::int32_t {
    None = 0,
    // User opened the overlay at least once
    Activation = 1 << 0,
};

bool hasKnowledge(Knowledge knowledge)
{
    QFlags current(static_cast<Knowledge>(
        getSettings()->overlayKnowledgeLevel.getValue()));
    return current.testFlag(knowledge);
}

void acquireKnowledge(Knowledge knowledge)
{
    QFlags current(static_cast<Knowledge>(
        getSettings()->overlayKnowledgeLevel.getValue()));
    current.setFlag(knowledge);
    getSettings()->overlayKnowledgeLevel = current;
}

void triggerFirstActivation(QWidget *parent)
{
    if (hasKnowledge(Knowledge::Activation))
    {
        return;
    }
    acquireKnowledge(Knowledge::Activation);

    auto welcomeText =
        u"Hey! It looks like this is the first time you're using the overlay. You can move the overlay holding SHIFT and dragging it with your mouse. To resize the window, drag on the bottom right corner."_s;
#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    auto actualShortcut =
        QKeySequence::fromString(getSettings()->overlayInertShortcut,
                                 QKeySequence::PortableText)
            .toString(QKeySequence::PortableText);
    welcomeText +=
        u"By default the overlay is interactive. To toggle the click-through mode, press %1 (customizable in the settings)."_s
            .arg(actualShortcut);
#endif

    auto *box =
        new QMessageBox(QMessageBox::Information, u"Chatterino - Overlay"_s,
                        welcomeText, QMessageBox::Ok, parent);
    box->open();
}

}  // namespace

namespace chatterino {

OverlayWindow::OverlayWindow(IndirectChannel channel)
    : QWidget(nullptr,
              Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , channel_(std::move(channel))
    , channelView_(nullptr)
    , interactAnimation_(this, "interactionProgress"_ba)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle(u"Chatterino - Overlay"_s);

    auto *grid = new QGridLayout(this);
    grid->addWidget(&this->channelView_, 0, 0);
    grid->addWidget(new InvisibleSizeGrip(this), 0, 0,
                    Qt::AlignBottom | Qt::AlignRight);
    grid->addWidget(&this->closeButton_, 0, 0, Qt::AlignTop | Qt::AlignRight);
    grid->setContentsMargins(0, 0, 0, 0);

    this->closeButton_.setButtonStyle(TitleBarButtonStyle::Close);
    this->closeButton_.setScaleIndependantSize(46, 30);
    this->closeButton_.hide();
    connect(&this->closeButton_, &TitleBarButton::leftClicked, [this]() {
        this->close();
    });
    this->closeButton_.setCursor(Qt::PointingHandCursor);

    this->channelView_.installEventFilter(this);
    this->channelView_.setChannel(this->channel_.get());
    this->channelView_.setColorVisitor([](MessageColors &colors, Theme *theme) {
        colors.applyOverlay(theme, getSettings()->overlayBackgroundOpacity);
    });
    this->channelView_.setAttribute(Qt::WA_TranslucentBackground);
    this->holder_.managedConnect(this->channel_.getChannelChanged(), [this]() {
        this->channelView_.setChannel(this->channel_.get());
    });

    this->setAutoFillBackground(false);
    this->resize(300, 500);
    this->move(QCursor::pos() - this->rect().center());
    this->setContentsMargins(0, 0, 0, 0);
    this->setAttribute(Qt::WA_TranslucentBackground);

    this->interactAnimation_.setStartValue(0.0);
    this->interactAnimation_.setEndValue(1.0);
    this->interactAnimation_.setDuration(150);

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
        this->holder_);
    settings->overlayBackgroundOpacity.connect(
        [this] {
            this->channelView_.updateColorTheme();
        },
        this->holder_, false);

    auto applyIt = [this](auto /*unused*/) {
        this->applyTheme();
    };
    settings->overlayShadowOffsetX.connect(applyIt, this->holder_, false);
    settings->overlayShadowOffsetY.connect(applyIt, this->holder_, false);
    settings->overlayShadowOpacity.connect(applyIt, this->holder_, false);
    settings->overlayShadowRadius.connect(applyIt, this->holder_, false);

    this->holder_.managedConnect(getTheme()->updated, [this] {
        this->applyTheme();
    });

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    getSettings()->overlayInertShortcut.connect(
        [this](const auto &value) {
            this->shortcut_ = std::make_unique<GlobalShortcut>(
                QKeySequence::fromString(value, QKeySequence::PortableText),
                this);
            QObject::connect(this->shortcut_.get(), &GlobalShortcut::activated,
                             this, [this] {
                                 this->setInert(!this->inert_);
                             });
        },
        this->holder_);
#endif

    triggerFirstActivation(this);
}

OverlayWindow::~OverlayWindow() = default;

void OverlayWindow::applyTheme()
{
    auto *theme = getTheme();
    auto *settings = getSettings();

    if (this->dropShadow_)
    {
        auto shadowColor = theme->overlayMessages.shadow.color;
        shadowColor.setAlpha(
            std::clamp(settings->overlayShadowOpacity.getValue(), 0, 255));
        this->dropShadow_->setColor(shadowColor);
        this->dropShadow_->setOffset(settings->overlayShadowOffsetX,
                                     settings->overlayShadowOffsetY);
        this->dropShadow_->setBlurRadius(settings->overlayShadowRadius);
    }
    this->update();
}

bool OverlayWindow::eventFilter(QObject * /*object*/, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::MouseButtonPress: {
            auto *evt = dynamic_cast<QMouseEvent *>(event);
            if (evt->modifiers().testFlag(Qt::ShiftModifier))
            {
                this->moving_ = true;
                this->moveOrigin_ = evt->globalPos();
                this->startInteraction();
                return true;
            }
            return false;
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
            auto shiftPressed = evt->modifiers().testFlag(Qt::ShiftModifier);
            if (!this->interacting_ && shiftPressed)
            {
                this->startInteraction();
                return true;
            }
            if (this->interacting_ && !shiftPressed)
            {
                this->endInteraction();
                return true;
            }

            if (this->moving_)
            {
                auto newPos = evt->globalPos() - this->moveOrigin_;
                this->move(newPos + this->pos());
                this->moveOrigin_ = evt->globalPos();
                return true;
            }
            if (this->interacting_)
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
}

void OverlayWindow::setOverrideCursor(const QCursor &cursor)
{
    this->channelView_.setCursor(cursor);
    this->setCursor(cursor);
}

void OverlayWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
    {
        this->startInteraction();
    }
}

void OverlayWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
    {
        this->endInteraction();
    }
}

void OverlayWindow::paintEvent(QPaintEvent * /*event*/)
{
#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    if (this->inert_)
    {
        return;
    }
#endif

    QPainter painter(this);
    QColor highlightColor(
        255, 255, 255, std::max(int(255.0 * this->interactionProgress()), 50));

    painter.setPen({highlightColor, 2});
    // outline
    auto bounds = this->rect();
    painter.drawRect(bounds);

    if (this->interactionProgress() <= 0.0)
    {
        return;
    }

    painter.setBrush(highlightColor);
    painter.setPen(Qt::transparent);

    // bottom resize triangle
    auto br = bounds.bottomRight();
    std::array<QPoint, 3> triangle = {br - QPoint{20, 0}, br,
                                      br - QPoint{0, 20}};
    painter.drawPolygon(triangle.data(), triangle.size());

    // close button
    auto buttonSize = this->closeButton_.size();
    painter.drawRect(
        QRect{bounds.topRight() - QPoint{buttonSize.width(), 0}, buttonSize});
}

double OverlayWindow::interactionProgress() const
{
    return this->interactionProgress_;
}

void OverlayWindow::setInteractionProgress(double progress)
{
    this->interactionProgress_ = progress;
    this->update();
}

void OverlayWindow::startInteraction()
{
#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    if (this->inert_)
    {
        return;
    }
#endif

    if (this->interacting_)
    {
        return;
    }

    this->interacting_ = true;
    if (this->interactAnimation_.state() != QPropertyAnimation::Stopped)
    {
        this->interactAnimation_.stop();
    }
    this->interactAnimation_.setDirection(QPropertyAnimation::Forward);
    this->interactAnimation_.start();
    this->setOverrideCursor(Qt::SizeAllCursor);
    this->closeButton_.show();
}

void OverlayWindow::endInteraction()
{
    if (!this->interacting_)
    {
        return;
    }

    this->interacting_ = false;
    if (this->interactAnimation_.state() != QPropertyAnimation::Stopped)
    {
        this->interactAnimation_.stop();
    }
    this->interactAnimation_.setDirection(QPropertyAnimation::Backward);
    this->interactAnimation_.start();
    this->setOverrideCursor(Qt::ArrowCursor);
    this->closeButton_.hide();
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

    auto *scrollbar = this->channelView_.scrollbar();
    scrollbar->setShowThumb(!inert);
    if (inert)
    {
        scrollbar->scrollToBottom();
    }
}

}  // namespace chatterino
