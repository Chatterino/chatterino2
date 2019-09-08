#include "Button.hpp"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QPainter>

#include "BaseTheme.hpp"
#include "util/FunctionEventFilter.hpp"

namespace AB_NAMESPACE {

Button::Button(BaseWidget *parent)
    : BaseWidget(parent)
{
    connect(&effectTimer_, &QTimer::timeout, this,
            &Button::onMouseEffectTimeout);

    this->effectTimer_.setInterval(20);
    this->effectTimer_.start();

    this->setMouseTracking(true);
}

void Button::setMouseEffectColor(boost::optional<QColor> color)
{
    this->mouseEffectColor_ = color;
}

void Button::setPixmap(const QPixmap &_pixmap)
{
    this->pixmap_ = _pixmap;
    this->update();
}

const QPixmap &Button::getPixmap() const
{
    return this->pixmap_;
}

void Button::setDim(Dim value)
{
    this->dimPixmap_ = value;

    this->update();
}

Button::Dim Button::getDim() const
{
    return this->dimPixmap_;
}

void Button::setEnable(bool value)
{
    this->enabled_ = value;

    this->update();
}

bool Button::getEnable() const
{
    return this->enabled_;
}

void Button::setEnableMargin(bool value)
{
    this->enableMargin_ = value;

    this->update();
}

bool Button::getEnableMargin() const
{
    return this->enableMargin_;
}

qreal Button::getCurrentDimAmount() const
{
    if (this->dimPixmap_ == Dim::None || this->mouseOver_)
        return 1;
    else if (this->dimPixmap_ == Dim::Some)
        return 0.7;
    else
        return 0.15;
}

void Button::setBorderColor(const QColor &color)
{
    this->borderColor_ = color;

    this->update();
}

const QColor &Button::getBorderColor() const
{
    return this->borderColor_;
}

void Button::setMenu(std::unique_ptr<QMenu> menu)
{
    this->menu_ = std::move(menu);

    this->menu_->installEventFilter(
        new FunctionEventFilter(this, [this](QObject *, QEvent *event) {
            if (event->type() == QEvent::Hide)
            {
                QTimer::singleShot(20, this,
                                   [this] { this->menuVisible_ = false; });
            }
            return false;
        }));
}

void Button::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!this->pixmap_.isNull())
    {
        painter.setOpacity(this->getCurrentDimAmount());

        QRect rect = this->rect();

        int margin = this->height() < 22 * this->scale() ? 3 : 6;

        int s = this->enableMargin_ ? int(margin * this->scale()) : 0;

        rect.moveLeft(s);
        rect.setRight(rect.right() - s - s);
        rect.moveTop(s);
        rect.setBottom(rect.bottom() - s - s);

        painter.drawPixmap(rect, this->pixmap_);

        painter.setOpacity(1);
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

    painter.setRenderHint(QPainter::HighQualityAntialiasing);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor c;

    if (this->mouseEffectColor_)
    {
        c = this->mouseEffectColor_.get();
    }
    else
    {
        c = this->theme->isLightTheme() ? QColor(0, 0, 0)
                                        : QColor(255, 255, 255);
    }

    if (this->hoverMultiplier_ > 0)
    {
        QRadialGradient gradient(QPointF(mousePos_), this->width() / 2);

        gradient.setColorAt(0, QColor(c.red(), c.green(), c.blue(),
                                      int(50 * this->hoverMultiplier_)));
        gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(),
                                      int(40 * this->hoverMultiplier_)));

        painter.fillRect(this->rect(), gradient);
    }

    for (auto effect : this->clickEffects_)
    {
        QRadialGradient gradient(effect.position.x(), effect.position.y(),
                                 effect.progress * qreal(width()) * 2,
                                 effect.position.x(), effect.position.y());

        gradient.setColorAt(0, QColor(c.red(), c.green(), c.blue(),
                                      int((1 - effect.progress) * 95)));
        gradient.setColorAt(0.9999, QColor(c.red(), c.green(), c.blue(),
                                           int((1 - effect.progress) * 95)));
        gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(), int(0)));

        painter.fillRect(this->rect(), gradient);
    }
}

void Button::enterEvent(QEvent *)
{
    this->mouseOver_ = true;
}

void Button::leaveEvent(QEvent *)
{
    this->mouseOver_ = false;
}

void Button::mousePressEvent(QMouseEvent *event)
{
    if (!this->enabled_)
    {
        return;
    }

    if (event->button() != Qt::LeftButton)
    {
        return;
    }

    this->clickEffects_.push_back(ClickEffect(event->pos()));

    this->mouseDown_ = true;

    emit this->leftMousePress();

    if (this->menu_ && !this->menuVisible_)
    {
        QTimer::singleShot(80, this, [this] { this->showMenu(); });
        this->mouseDown_ = false;
        this->mouseOver_ = false;
    }
}

void Button::mouseReleaseEvent(QMouseEvent *event)
{
    if (!this->enabled_)
        return;

    if (event->button() == Qt::LeftButton)
    {
        this->mouseDown_ = false;

        if (this->rect().contains(event->pos()))
            emit leftClicked();
    }

    emit clicked(event->button());
}

void Button::mouseMoveEvent(QMouseEvent *event)
{
    if (this->enabled_)
    {
        this->mousePos_ = event->pos();

        this->update();
    }
}

void Button::onMouseEffectTimeout()
{
    bool performUpdate = false;

    if (selected_)
    {
        if (this->hoverMultiplier_ != 0)
        {
            this->hoverMultiplier_ =
                std::max(0.0, this->hoverMultiplier_ - 0.1);
            performUpdate = true;
        }
    }
    else if (mouseOver_)
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

    if (this->clickEffects_.size() != 0)
    {
        performUpdate = true;

        for (auto it = this->clickEffects_.begin();
             it != this->clickEffects_.end();)
        {
            it->progress += mouseDown_ ? 0.02 : 0.07;

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
        return;

    auto point = [this] {
        auto bounds = QApplication::desktop()->availableGeometry(this);

        auto point = this->mapToGlobal(
            QPoint(this->width() - this->menu_->width(), this->height()));

        if (point.y() + this->menu_->height() > bounds.bottom())
        {
            point.setY(point.y() - this->menu_->height() - this->height());
        }

        return point;
    };

    this->menu_->popup(point());
    this->menu_->move(point());
    this->menuVisible_ = true;
}

}  // namespace AB_NAMESPACE
