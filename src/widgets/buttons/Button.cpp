#include "widgets/buttons/Button.hpp"

#include "singletons/Theme.hpp"
#include "util/FunctionEventFilter.hpp"

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QScreen>

namespace chatterino {

Button::Button(BaseWidget *parent)
    : BaseWidget(parent)
{
    connect(&effectTimer_, &QTimer::timeout, this,
            &Button::onMouseEffectTimeout);

    this->effectTimer_.setInterval(20);
    this->effectTimer_.start();

    this->setMouseTracking(true);
}

bool Button::enabled() const noexcept
{
    return this->enabled_;
}

void Button::setEnabled(bool enabled)
{
    this->enabled_ = enabled;

    this->update();
}

bool Button::mouseOver() const noexcept
{
    return this->mouseOver_;
}

bool Button::leftMouseButtonDown() const noexcept
{
    return this->leftMouseButtonDown_;
}

bool Button::menuVisible() const noexcept
{
    return this->menuVisible_;
}

QColor Button::borderColor() const noexcept
{
    return this->borderColor_;
}

void Button::setBorderColor(const QColor &color)
{
    this->borderColor_ = color;

    this->update();
}

std::optional<QColor> Button::mouseEffectColor() const
{
    return this->mouseEffectColor_;
}

void Button::setMouseEffectColor(std::optional<QColor> color)
{
    this->mouseEffectColor_ = color;
}

QMenu *Button::menu() const
{
    return this->menu_.get();
}

void Button::setMenu(std::unique_ptr<QMenu> menu)
{
    if (this->menu_)
    {
        this->menu_.release()->deleteLater();
    }

    this->menu_ = std::move(menu);

    this->menu_->installEventFilter(
        new FunctionEventFilter(this, [this](QObject *, QEvent *event) {
            if (event->type() == QEvent::Hide)
            {
                QTimer::singleShot(20, this, [this] {
                    this->menuVisible_ = false;
                });
            }
            return false;
        }));
}

void Button::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    this->paintButton(painter);
}

void Button::invalidateContent()
{
    this->pixmapValid_ = false;
    this->update();
}

bool Button::contentCacheEnabled() const noexcept
{
    return this->cachePixmap_;
}

void Button::setContentCacheEnabled(bool enabled)
{
    if (this->cachePixmap_ == enabled)
    {
        return;
    }

    if (!enabled)
    {
        this->cachedPixmap_ = {};
    }
    this->cachePixmap_ = enabled;
}

bool Button::opaqueContent() const noexcept
{
    return this->opaqueContent_;
}

void Button::setOpaqueContent(bool opaqueContent)
{
    this->opaqueContent_ = opaqueContent;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void Button::enterEvent(QEnterEvent * /*event*/)
#else
void Button::enterEvent(QEvent * /*event*/)
#endif
{
    if (!this->mouseOver_)
    {
        this->mouseOver_ = true;
        this->update();
        this->mouseOverUpdated();
    }
}

void Button::leaveEvent(QEvent * /*event*/)
{
    if (this->mouseOver_)
    {
        this->mouseOver_ = false;
        this->update();
        this->mouseOverUpdated();
    }
}

void Button::mousePressEvent(QMouseEvent *event)
{
    if (!this->enabled_)
    {
        return;
    }

    switch (event->button())
    {
        case Qt::MouseButton::LeftButton: {
            this->leftMouseButtonDown_ = true;

            this->addClickEffect(event->pos());

            this->leftMousePress();

            if (this->menu_ && !this->menuVisible_)
            {
                QTimer::singleShot(80, this, [this] {
                    this->showMenu();
                });
                this->leftMouseButtonDown_ = false;
                this->mouseOver_ = false;
            }
        }
        break;
        case Qt::MouseButton::RightButton: {
            this->rightMouseButtonDown_ = true;
        }
        break;
        case Qt::MouseButton::MiddleButton: {
            this->middleMouseButtonDown_ = true;
        }
        break;

        default:
            // Unsupported button
            return;
    }
}

void Button::mouseReleaseEvent(QMouseEvent *event)
{
    // Reset the "mouse button down" state of the released button and store
    // whether the button was in the down state when this event fired
    bool hadCorrectButtonPressed = false;
    switch (event->button())
    {
        case Qt::MouseButton::LeftButton: {
            hadCorrectButtonPressed = this->leftMouseButtonDown_;
            this->leftMouseButtonDown_ = false;
        }
        break;
        case Qt::MouseButton::RightButton: {
            hadCorrectButtonPressed = this->rightMouseButtonDown_;
            this->rightMouseButtonDown_ = false;
        }
        break;
        case Qt::MouseButton::MiddleButton: {
            hadCorrectButtonPressed = this->middleMouseButtonDown_;
            this->middleMouseButtonDown_ = false;
        }
        break;

        default:
            // Unsupported button
            return;
    }

    if (!this->enabled_)
    {
        return;
    }

    bool isInside = this->rect().contains(event->pos());

    if (isInside && hadCorrectButtonPressed)
    {
        if (event->button() == Qt::LeftButton)
        {
            leftClicked();
        }

        clicked(event->button());
    }
}

void Button::mouseMoveEvent(QMouseEvent *event)
{
    if (this->enabled_)
    {
        this->mousePos_ = event->pos();

        this->update();
    }
}

void Button::addClickEffect(QPoint position)
{
    this->clickEffects_.emplace_back(position);
}

void Button::onMouseEffectTimeout()
{
    bool performUpdate = false;

    if (mouseOver_)
    {
        if (this->hoverMultiplier_ != 1)
        {
            this->hoverMultiplier_ =
                std::min(1.0, this->hoverMultiplier_ + 0.5);
            performUpdate = true;
        }
    }
    else
    {
        if (this->hoverMultiplier_ != 0)
        {
            this->hoverMultiplier_ =
                std::max(0.0, this->hoverMultiplier_ - 0.3);
            performUpdate = true;
        }
    }

    if (!this->clickEffects_.empty())
    {
        performUpdate = true;

        for (auto it = this->clickEffects_.begin();
             it != this->clickEffects_.end();)
        {
            it->progress += this->leftMouseButtonDown_ ? 0.02 : 0.07;

            if (it->progress >= 1.0)
            {
                it = this->clickEffects_.erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    if (performUpdate)
    {
        update();
    }
}

void Button::showMenu()
{
    if (!this->menu_)
    {
        return;
    }

    auto menuSizeHint = this->menu_->sizeHint();
    auto point = this->mapToGlobal(
        QPoint(this->width() - menuSizeHint.width(), this->height()));

    auto *screen = QApplication::screenAt(point);
    if (screen == nullptr)
    {
        screen = QApplication::primaryScreen();
    }
    auto bounds = screen->availableGeometry();

    if (point.y() + menuSizeHint.height() > bounds.bottom())
    {
        // Menu doesn't fit going down, flip it to go up instead
        point.setY(point.y() - menuSizeHint.height() - this->height());
    }

    this->menu_->popup(point);
    this->menuVisible_ = true;
}

void Button::paintButton(QPainter &painter)
{
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (this->cachePixmap_)
    {
        if (this->cachedPixmap_.size() / this->devicePixelRatio() !=
            this->size())
        {
            this->cachedPixmap_ =
                QPixmap(this->size() * this->devicePixelRatio());
            this->cachedPixmap_.setDevicePixelRatio(this->devicePixelRatio());
            this->pixmapValid_ = false;
        }

        if (!this->pixmapValid_)
        {
            if (!this->opaqueContent_)
            {
                this->cachedPixmap_.fill(Qt::transparent);
            }

            {
                QPainter pixmapPainter(&this->cachedPixmap_);
                this->paintContent(pixmapPainter);
            }

            this->pixmapValid_ = true;
        }
        painter.drawPixmap(this->rect(), this->cachedPixmap_,
                           {{}, this->cachedPixmap_.size()});
    }
    else
    {
        this->paintContent(painter);
    }

    this->fancyPaint(painter);

    if (this->borderColor_.isValid())
    {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(this->borderColor_);
        painter.drawRect(0, 0, this->width() - 1, this->height() - 1);
    }
}

void Button::fancyPaint(QPainter &painter)
{
    if (!this->enabled_)
    {
        return;
    }

    painter.setRenderHint(QPainter::Antialiasing);
    QColor c;

    if (this->mouseEffectColor_)
    {
        c = *this->mouseEffectColor_;
    }
    else
    {
        c = this->theme->isLightTheme() ? QColor(0, 0, 0)
                                        : QColor(255, 255, 255);
    }

    if (this->hoverMultiplier_ > 0)
    {
        QRadialGradient gradient(QPointF(mousePos_),
                                 static_cast<qreal>(this->width()) / 2.0);

        gradient.setColorAt(0, QColor(c.red(), c.green(), c.blue(),
                                      int(50 * this->hoverMultiplier_)));
        gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(),
                                      int(40 * this->hoverMultiplier_)));

        painter.fillRect(this->rect(), gradient);
    }

    for (auto effect : this->clickEffects_)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(c.red(), c.green(), c.blue(),
                                int((1 - effect.progress) * 95)));
        painter.drawEllipse(QPointF(effect.position),
                            effect.progress * qreal(width()) * 2,
                            effect.progress * qreal(width()) * 2);
    }
}

}  // namespace chatterino
