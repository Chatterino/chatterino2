#include "widgets/splits/SplitOverlay.hpp"

#include "Application.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QEvent>
#include <QGraphicsBlurEffect>
#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QPainter>
#include <QPushButton>

namespace {

using namespace chatterino;

class ButtonEventFilter : public QObject
{
    SplitOverlay *parent;
    SplitOverlayButton buttonType;

public:
    ButtonEventFilter(SplitOverlay *_parent, SplitOverlayButton _buttonType);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

ButtonEventFilter::ButtonEventFilter(SplitOverlay *_parent,
                                     SplitOverlayButton _buttonType)
    : QObject(_parent)
    , parent(_parent)
    , buttonType(_buttonType)
{
}

bool ButtonEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::Enter: {
            auto *effect = dynamic_cast<QGraphicsOpacityEffect *>(
                ((QWidget *)watched)->graphicsEffect());

            if (effect != nullptr)
            {
                effect->setOpacity(0.99);
            }

            this->parent->setHoveredButton(this->buttonType);
        }
        break;

        case QEvent::Leave: {
            auto *effect = dynamic_cast<QGraphicsOpacityEffect *>(
                ((QWidget *)watched)->graphicsEffect());

            if (effect != nullptr)
            {
                effect->setOpacity(0.7);
            }

            this->parent->setHoveredButton({});
        }
        break;

        case QEvent::MouseButtonPress: {
            if (this->buttonType == SplitOverlayButton::Move)
            {
                auto *mouseEvent = dynamic_cast<QMouseEvent *>(event);
                assert(mouseEvent != nullptr);
                if (mouseEvent->button() == Qt::LeftButton)
                {
                    this->parent->dragPressed();
                }
                return true;
            }
        }
        break;

        case QEvent::MouseButtonRelease: {
            switch (this->buttonType)
            {
                case SplitOverlayButton::Move:
                    break;

                case SplitOverlayButton::Left: {
                    this->parent->createSplitPressed(SplitDirection::Left);
                }
                break;

                case SplitOverlayButton::Up: {
                    this->parent->createSplitPressed(SplitDirection::Above);
                }
                break;

                case SplitOverlayButton::Right: {
                    this->parent->createSplitPressed(SplitDirection::Right);
                }
                break;

                case SplitOverlayButton::Down: {
                    this->parent->createSplitPressed(SplitDirection::Below);
                }
                break;
            }
        }
        break;

        default:;
    }
    return QObject::eventFilter(watched, event);
}

}  // namespace

namespace chatterino {

SplitOverlay::SplitOverlay(Split *parent)
    : BaseWidget(parent)
    , split_(parent)
{
    auto *layout = new QGridLayout(this);
    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(1);

    layout->setRowStretch(1, 1);
    layout->setRowStretch(3, 1);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    this->move_ = new QPushButton(getResources().split.move, {});
    this->left_ = new QPushButton(getResources().split.left, {});
    this->right_ = new QPushButton(getResources().split.right, {});
    this->up_ = new QPushButton(getResources().split.up, {});
    this->down_ = new QPushButton(getResources().split.down, {});

    this->move_->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    this->left_->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    this->right_->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    this->up_->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    this->down_->setGraphicsEffect(new QGraphicsOpacityEffect(this));

    this->move_->setFlat(true);
    this->left_->setFlat(true);
    this->right_->setFlat(true);
    this->up_->setFlat(true);
    this->down_->setFlat(true);

    layout->addWidget(this->move_, 2, 2);
    layout->addWidget(this->left_, 2, 0);
    layout->addWidget(this->right_, 2, 4);
    layout->addWidget(this->up_, 0, 2);
    layout->addWidget(this->down_, 4, 2);

    this->move_->installEventFilter(
        new ButtonEventFilter(this, SplitOverlayButton::Move));
    this->left_->installEventFilter(
        new ButtonEventFilter(this, SplitOverlayButton::Left));
    this->right_->installEventFilter(
        new ButtonEventFilter(this, SplitOverlayButton::Right));
    this->up_->installEventFilter(
        new ButtonEventFilter(this, SplitOverlayButton::Up));
    this->down_->installEventFilter(
        new ButtonEventFilter(this, SplitOverlayButton::Down));

    this->move_->setFocusPolicy(Qt::NoFocus);
    this->left_->setFocusPolicy(Qt::NoFocus);
    this->right_->setFocusPolicy(Qt::NoFocus);
    this->up_->setFocusPolicy(Qt::NoFocus);
    this->down_->setFocusPolicy(Qt::NoFocus);

    this->move_->setCursor(Qt::SizeAllCursor);
    this->left_->setCursor(Qt::PointingHandCursor);
    this->right_->setCursor(Qt::PointingHandCursor);
    this->up_->setCursor(Qt::PointingHandCursor);
    this->down_->setCursor(Qt::PointingHandCursor);

    this->setMouseTracking(true);

    this->setCursor(Qt::ArrowCursor);
}

void SplitOverlay::setHoveredButton(
    std::optional<SplitOverlayButton> hoveredButton)
{
    this->hoveredButton_ = hoveredButton;
    this->update();
}

void SplitOverlay::dragPressed()
{
    this->split_->drag();
}

void SplitOverlay::createSplitPressed(SplitDirection direction)
{
    this->split_->insertSplitRequested.invoke(direction, this->split_);
    this->hide();
}

void SplitOverlay::scaleChangedEvent(float newScale)
{
    int a = int(newScale * 30);
    QSize size(a, a);

    this->move_->setIconSize(size);
    this->left_->setIconSize(size);
    this->right_->setIconSize(size);
    this->up_->setIconSize(size);
    this->down_->setIconSize(size);
    BaseWidget::scaleChangedEvent(newScale);
}

void SplitOverlay::paintEvent(QPaintEvent *event)
{
    (void)event;

    QPainter painter(this);
    if (this->theme->isLightTheme())
    {
        painter.fillRect(this->rect(), QColor(255, 255, 255, 200));
    }
    else
    {
        painter.fillRect(this->rect(), QColor(0, 0, 0, 150));
    }

    if (!this->hoveredButton_.has_value())
    {
        return;
    }

    QRect rect;
    auto hoveredButton = this->hoveredButton_.value();
    switch (hoveredButton)
    {
        case SplitOverlayButton::Left: {
            rect = QRect(0, 0, this->width() / 2, this->height());
        }
        break;

        case SplitOverlayButton::Right: {
            rect =
                QRect(this->width() / 2, 0, this->width() / 2, this->height());
        }
        break;

        case SplitOverlayButton::Up: {
            rect = QRect(0, 0, this->width(), this->height() / 2);
        }
        break;

        case SplitOverlayButton::Down: {
            rect =
                QRect(0, this->height() / 2, this->width(), this->height() / 2);
        }
        break;

        default:;
    }

    if (!rect.isNull())
    {
        rect.setRight(rect.right() - 1);
        rect.setBottom(rect.bottom() - 1);
        painter.setPen(getApp()->getThemes()->splits.dropPreviewBorder);
        painter.setBrush(getApp()->getThemes()->splits.dropPreview);
        painter.drawRect(rect);
    }
}

void SplitOverlay::resizeEvent(QResizeEvent *event)
{
    const auto currentScale = this->scale();
    const auto minimumWidth = 150.F * currentScale;
    const auto minimumHeight = 150.F * currentScale;

    const auto wideEnough = event->size().width() > minimumWidth;
    const auto highEnough = event->size().height() > minimumHeight;

    this->left_->setVisible(wideEnough);
    this->right_->setVisible(wideEnough);
    this->up_->setVisible(highEnough);
    this->down_->setVisible(highEnough);
}

void SplitOverlay::mouseMoveEvent(QMouseEvent *event)
{
    BaseWidget::mouseMoveEvent(event);

    //    if ((QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier) ==
    //    Qt::AltModifier) {
    //        this->hide();
    //    }
}

}  // namespace chatterino
