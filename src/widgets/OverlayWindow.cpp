#include "widgets/OverlayWindow.hpp"

#include "BaseSettings.hpp"
#include "controllers/hotkeys/GlobalShortcut.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/TitlebarButton.hpp"
#include "widgets/splits/Split.hpp"

#include <QBoxLayout>
#include <QCursor>
#include <qgraphicseffect.h>
#include <QGraphicsEffect>
#include <QGridLayout>
#include <QKeySequence>
#include <QSizeGrip>

#include <array>

namespace {

class Grippy : public QSizeGrip
{
public:
    Grippy(QWidget *parent)
        : QSizeGrip(parent)
    {
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
    }
};

}  // namespace

namespace chatterino {

OverlayWindow::OverlayWindow(IndirectChannel channel, Split *split)
    : QWidget(nullptr,
              Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , channel_(std::move(channel))
    , channelView_(nullptr, split)
    , interactAnimation_(this, QByteArrayLiteral("interactionProgress"))
{
    auto *grid = new QGridLayout(this);
    grid->addWidget(&this->channelView_, 0, 0);
    grid->addWidget(new Grippy(this), 0, 0, Qt::AlignBottom | Qt::AlignRight);
    grid->addWidget(&this->closeButton_, 0, 0, Qt::AlignTop | Qt::AlignRight);
    grid->setContentsMargins(0, 0, 0, 0);

    this->closeButton_.setButtonStyle(TitleBarButtonStyle::Close);
    this->closeButton_.setScaleIndependantSize(46, 30);
    this->closeButton_.hide();
    connect(&this->closeButton_, &TitleBarButton::leftClicked, [this]() {
        this->close();
    });

    this->channelView_.installEventFilter(this);
    this->channelView_.setChannel(this->channel_.get());
    this->channelView_.setColorVisitor([](MessageColors &colors, Theme *theme) {
        colors.applyOverlay(theme);
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

    auto applyDropShadowTheme = [this]() {
        auto *theme = getTheme();
        this->dropShadow_->setColor(theme->overlayMessages.shadow.color);
        this->dropShadow_->setOffset(theme->overlayMessages.shadow.offset);
        this->dropShadow_->setBlurRadius(
            theme->overlayMessages.shadow.blurRadius);
    };
    getSettings()->enableOverlayShadow.connect(
        [this, applyDropShadowTheme](bool value) {
            if (value)
            {
                this->dropShadow_ = new QGraphicsDropShadowEffect;
                applyDropShadowTheme();
                this->channelView_.setGraphicsEffect(this->dropShadow_);
            }
            else
            {
                this->channelView_.setGraphicsEffect(nullptr);
            }
        },
        this->holder_);

    applyDropShadowTheme();
    this->holder_.managedConnect(getTheme()->updated, applyDropShadowTheme);

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT
    getSettings()->overlayInertShortcut.connect(
        [this](const auto &value) {
            this->shortcut_ = std::make_unique<GlobalShortcut>(
                QKeySequence::fromString(value, QKeySequence::PortableText),
                this);
            QObject::connect(this->shortcut_.get(), &GlobalShortcut::activated,
                             this, [this] {
                                 qDebug() << "!!!!!!!";
                                 this->inert_ = !this->inert_;
                                 this->setWindowFlag(
                                     Qt::WindowTransparentForInput,
                                     this->inert_);
                                 if (this->isHidden())
                                 {
                                     this->show();
                                 }
                                 this->endInteraction();
                             });
        },
        this->holder_);
#endif
}

OverlayWindow::~OverlayWindow() = default;

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
            if (this->moving_)
            {
                auto newPos = evt->globalPos() - this->moveOrigin_;
                this->move(newPos + this->pos());
                this->moveOrigin_ = evt->globalPos();
                return true;
            }
            return false;
        }
        break;
        default:
            return false;
    }
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
    if (this->inert_)
    {
        return;
    }

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
}

void OverlayWindow::startInteraction()
{
    if (this->inert_ || this->interacting_)
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
    this->setCursor(Qt::DragMoveCursor);
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
    this->setCursor(Qt::ArrowCursor);
    this->closeButton_.hide();
}

}  // namespace chatterino
