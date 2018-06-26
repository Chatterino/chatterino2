#include "SplitOverlay.hpp"

#include <QEvent>
#include <QGraphicsBlurEffect>
#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QPainter>
#include <QPushButton>

#include "Application.hpp"
#include "singletons/ResourceManager.hpp"
#include "widgets/Split.hpp"
#include "widgets/SplitContainer.hpp"

namespace chatterino {
namespace widgets {

SplitOverlay::SplitOverlay(Split *parent)
    : BaseWidget(parent)
    , split(parent)
{
    QGridLayout *layout = new QGridLayout(this);
    this->_layout = layout;
    layout->setMargin(1);
    layout->setSpacing(1);

    layout->setRowStretch(1, 1);
    layout->setRowStretch(3, 1);
    layout->setColumnStretch(1, 1);
    layout->setColumnStretch(3, 1);

    QPushButton *move = new QPushButton(getApp()->resources->split.move, QString());
    QPushButton *left = this->_left = new QPushButton(getApp()->resources->split.left, QString());
    QPushButton *right = this->_right =
        new QPushButton(getApp()->resources->split.right, QString());
    QPushButton *up = this->_up = new QPushButton(getApp()->resources->split.up, QString());
    QPushButton *down = this->_down = new QPushButton(getApp()->resources->split.down, QString());

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
    if (this->themeManager->isLightTheme()) {
        painter.fillRect(this->rect(), QColor(255, 255, 255, 200));
    } else {
        painter.fillRect(this->rect(), QColor(0, 0, 0, 150));
    }

    QRect rect;
    switch (this->hoveredElement) {
        case SplitLeft: {
            rect = QRect(0, 0, this->width() / 2, this->height());
        } break;

        case SplitRight: {
            rect = QRect(this->width() / 2, 0, this->width() / 2, this->height());
        } break;

        case SplitUp: {
            rect = QRect(0, 0, this->width(), this->height() / 2);
        } break;

        case SplitDown: {
            rect = QRect(0, this->height() / 2, this->width(), this->height() / 2);
        } break;

        default:;
    }

    rect.setRight(rect.right() - 1);
    rect.setBottom(rect.bottom() - 1);

    if (!rect.isNull()) {
        painter.setPen(getApp()->themes->splits.dropPreviewBorder);
        painter.setBrush(getApp()->themes->splits.dropPreview);
        painter.drawRect(rect);
    }
}

void SplitOverlay::resizeEvent(QResizeEvent *event)
{
    float _scale = this->getScale();
    bool wideEnough = event->size().width() > 150 * _scale;
    bool highEnough = event->size().height() > 150 * _scale;

    this->_left->setVisible(wideEnough);
    this->_right->setVisible(wideEnough);
    this->_up->setVisible(highEnough);
    this->_down->setVisible(highEnough);
}

void SplitOverlay::mouseMoveEvent(QMouseEvent *event)
{
    BaseWidget::mouseMoveEvent(event);

    //    qDebug() << QGuiApplication::queryKeyboardModifiers();

    //    if ((QGuiApplication::queryKeyboardModifiers() & Qt::AltModifier) == Qt::AltModifier) {
    //        this->hide();
    //    }
}

SplitOverlay::ButtonEventFilter::ButtonEventFilter(SplitOverlay *_parent, HoveredElement _element)
    : QObject(_parent)
    , parent(_parent)
    , hoveredElement(_element)
{
}

bool SplitOverlay::ButtonEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
        case QEvent::Enter: {
            QGraphicsOpacityEffect *effect =
                dynamic_cast<QGraphicsOpacityEffect *>(((QWidget *)watched)->graphicsEffect());

            if (effect != nullptr) {
                effect->setOpacity(0.99);
            }

            this->parent->hoveredElement = this->hoveredElement;
            this->parent->update();
        } break;
        case QEvent::Leave: {
            QGraphicsOpacityEffect *effect =
                dynamic_cast<QGraphicsOpacityEffect *>(((QWidget *)watched)->graphicsEffect());

            if (effect != nullptr) {
                effect->setOpacity(0.7);
            }

            this->parent->hoveredElement = HoveredElement::None;
            this->parent->update();
        } break;
        case QEvent::MouseButtonPress: {
            if (this->hoveredElement == HoveredElement::SplitMove) {
                QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    this->parent->split->drag();
                }
                return true;
            }
        } break;
        case QEvent::MouseButtonRelease: {
            if (this->hoveredElement != HoveredElement::SplitMove) {
                SplitContainer *container = this->parent->split->getContainer();

                if (container != nullptr) {
                    auto *_split = new Split(container);
                    auto dir = SplitContainer::Direction(this->hoveredElement +
                                                         SplitContainer::Left - SplitLeft);
                    container->insertSplit(_split, dir, this->parent->split);
                    this->parent->hide();
                }
            }
        } break;
        default:;
    }
    return QObject::eventFilter(watched, event);
}

}  // namespace widgets
}  // namespace chatterino
