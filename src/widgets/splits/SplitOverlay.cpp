#include "SplitOverlay.hpp"

#include <QEvent>
#include <QGraphicsBlurEffect>
#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QPainter>
#include <QPushButton>

#include "Application.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Theme.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

namespace chatterino {

SplitOverlay::SplitOverlay(Split *parent)
    : BaseWidget(parent)
    , split_(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    this->layout_ = layout;
    layout->setMargin(1);
    layout->setSpacing(1);

    layout->setRowStretch(1, 1);
    layout->setRowStretch(3, 1);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    auto *move = new QPushButton(getResources().split.move, QString());
    auto *left = this->left_ =
        new QPushButton(getResources().split.left, QString());
    auto *right = this->right_ =
        new QPushButton(getResources().split.right, QString());
    auto *up = this->up_ = new QPushButton(getResources().split.up, QString());
    auto *down = this->down_ =
        new QPushButton(getResources().split.down, QString());

    move->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    left->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    right->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    up->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    down->setGraphicsEffect(new QGraphicsOpacityEffect(this));

    move->setFlat(true);
    left->setFlat(true);
    right->setFlat(true);
    up->setFlat(true);
    down->setFlat(true);

    layout->addWidget(move, 2, 2);
    layout->addWidget(left, 2, 0);
    layout->addWidget(right, 2, 4);
    layout->addWidget(up, 0, 2);
    layout->addWidget(down, 4, 2);

    move->installEventFilter(new ButtonEventFilter(this, SplitMove));
    left->installEventFilter(new ButtonEventFilter(this, SplitLeft));
    right->installEventFilter(new ButtonEventFilter(this, SplitRight));
    up->installEventFilter(new ButtonEventFilter(this, SplitUp));
    down->installEventFilter(new ButtonEventFilter(this, SplitDown));

    move->setFocusPolicy(Qt::NoFocus);
    left->setFocusPolicy(Qt::NoFocus);
    right->setFocusPolicy(Qt::NoFocus);
    up->setFocusPolicy(Qt::NoFocus);
    down->setFocusPolicy(Qt::NoFocus);

    move->setCursor(Qt::SizeAllCursor);
    left->setCursor(Qt::PointingHandCursor);
    right->setCursor(Qt::PointingHandCursor);
    up->setCursor(Qt::PointingHandCursor);
    down->setCursor(Qt::PointingHandCursor);

    this->managedConnect(this->scaleChanged, [=](float _scale) {
        int a = int(_scale * 30);
        QSize size(a, a);

        move->setIconSize(size);
        left->setIconSize(size);
        right->setIconSize(size);
        up->setIconSize(size);
        down->setIconSize(size);
    });

    this->setMouseTracking(true);

    this->setCursor(Qt::ArrowCursor);
}

void SplitOverlay::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (this->theme->isLightTheme())
    {
        painter.fillRect(this->rect(), QColor(255, 255, 255, 200));
    }
    else
    {
        painter.fillRect(this->rect(), QColor(0, 0, 0, 150));
    }

    QRect rect;
    switch (this->hoveredElement_)
    {
        case SplitLeft: {
            rect = QRect(0, 0, this->width() / 2, this->height());
        }
        break;

        case SplitRight: {
            rect =
                QRect(this->width() / 2, 0, this->width() / 2, this->height());
        }
        break;

        case SplitUp: {
            rect = QRect(0, 0, this->width(), this->height() / 2);
        }
        break;

        case SplitDown: {
            rect =
                QRect(0, this->height() / 2, this->width(), this->height() / 2);
        }
        break;

        default:;
    }

    rect.setRight(rect.right() - 1);
    rect.setBottom(rect.bottom() - 1);

    if (!rect.isNull())
    {
        painter.setPen(getApp()->themes->splits.dropPreviewBorder);
        painter.setBrush(getApp()->themes->splits.dropPreview);
        painter.drawRect(rect);
    }
}

void SplitOverlay::resizeEvent(QResizeEvent *event)
{
    float _scale = this->scale();
    bool wideEnough = event->size().width() > 150 * _scale;
    bool highEnough = event->size().height() > 150 * _scale;

    this->left_->setVisible(wideEnough);
    this->right_->setVisible(wideEnough);
    this->up_->setVisible(highEnough);
    this->down_->setVisible(highEnough);
}

void SplitOverlay::mouseMoveEvent(QMouseEvent *event)
{
    BaseWidget::mouseMoveEvent(event);

    //    qDebug() << QGuiApplication::queryKeyboardModifiers();

    //    if ((QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier) ==
    //    Qt::AltModifier) {
    //        this->hide();
    //    }
}

SplitOverlay::ButtonEventFilter::ButtonEventFilter(SplitOverlay *_parent,
                                                   HoveredElement _element)
    : QObject(_parent)
    , parent(_parent)
    , hoveredElement(_element)
{
}

bool SplitOverlay::ButtonEventFilter::eventFilter(QObject *watched,
                                                  QEvent *event)
{
    switch (event->type())
    {
        case QEvent::Enter: {
            QGraphicsOpacityEffect *effect =
                dynamic_cast<QGraphicsOpacityEffect *>(
                    ((QWidget *)watched)->graphicsEffect());

            if (effect != nullptr)
            {
                effect->setOpacity(0.99);
            }

            this->parent->hoveredElement_ = this->hoveredElement;
            this->parent->update();
        }
        break;
        case QEvent::Leave: {
            QGraphicsOpacityEffect *effect =
                dynamic_cast<QGraphicsOpacityEffect *>(
                    ((QWidget *)watched)->graphicsEffect());

            if (effect != nullptr)
            {
                effect->setOpacity(0.7);
            }

            this->parent->hoveredElement_ = HoveredElement::None;
            this->parent->update();
        }
        break;
        case QEvent::MouseButtonPress: {
            if (this->hoveredElement == HoveredElement::SplitMove)
            {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::LeftButton)
                {
                    this->parent->split_->drag();
                }
                return true;
            }
        }
        break;
        case QEvent::MouseButtonRelease: {
            if (this->hoveredElement != HoveredElement::SplitMove)
            {
                SplitContainer *container =
                    this->parent->split_->getContainer();

                if (container != nullptr)
                {
                    auto *_split = new Split(container);
                    auto dir = SplitContainer::Direction(this->hoveredElement +
                                                         SplitContainer::Left -
                                                         SplitLeft);
                    container->insertSplit(_split, dir, this->parent->split_);
                    this->parent->hide();
                }
            }
        }
        break;
        default:;
    }
    return QObject::eventFilter(watched, event);
}

}  // namespace chatterino
