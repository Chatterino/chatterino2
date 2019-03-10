#include "FlatButton.hpp"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QPainter>
#include <QStyleOption>

#include "ab/Row.hpp"
#include "ab/util/Benchmark.hpp"
#include "ab/util/MakeWidget.hpp"
#include "util/FunctionEventFilter.hpp"

namespace ab
{
    FlatButton::FlatButton(BaseWidget* parent)
        : BaseWidget(parent)
    {
        connect(&effectTimer_, &QTimer::timeout, this,
            &FlatButton::onMouseEffectTimeout);

        this->effectTimer_.setInterval(20);
        this->effectTimer_.start();

        this->setMouseTracking(true);
    }

    void FlatButton::setPixmap(const QPixmap& _pixmap)
    {
        this->pixmap_ = _pixmap;
        this->pixmapScaled_ = {};
        this->update();
    }

    const QPixmap& FlatButton::getPixmap() const
    {
        return this->pixmap_;
    }

    void FlatButton::setDim(bool value)
    {
        this->dimPixmap_ = value;

        this->update();
    }

    bool FlatButton::getDim() const
    {
        return this->dimPixmap_;
    }

    void FlatButton::setEnable(bool value)
    {
        this->enabled_ = value;

        this->update();
    }

    bool FlatButton::getEnable() const
    {
        return this->enabled_;
    }

    void FlatButton::setEnableMargin(bool value)
    {
        this->enableMargin_ = value;

        this->update();
    }

    bool FlatButton::getEnableMargin() const
    {
        return this->enableMargin_;
    }

    void FlatButton::setRipple(bool value)
    {
        this->ripple_ = value;
    }

    bool FlatButton::ripple()
    {
        return this->ripple_;
    }

    void FlatButton::setHover(bool value)
    {
        this->hover_ = value;
    }

    bool FlatButton::hover()
    {
        return this->hover_;
    }

    qreal FlatButton::getCurrentDimAmount() const
    {
        return this->dimPixmap_ && !this->mouseOver_ ? 0.7 : 1;
    }

    void FlatButton::setMenu(std::unique_ptr<QMenu> menu)
    {
        this->menu_ = std::move(menu);

        this->menu_->installEventFilter(
            new FunctionEventFilter(this, [this](QObject*, QEvent* event) {
                if (event->type() == QEvent::Hide)
                {
                    QTimer::singleShot(
                        20, this, [this] { this->menuVisible_ = false; });
                }
                return false;
            }));
    }

    void FlatButton::setChild(QWidget* widget)
    {
        assert(!this->layout());

        this->setLayout(makeLayout<Row>({widget}));
    }

    void FlatButton::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        this->paint(painter);
    }

    void FlatButton::paint(QPainter& painter)
    {
        QStyleOptionFrame option;
        option.initFrom(this);
        this->style()->drawPrimitive(
            QStyle::PE_Widget, &option, &painter, this);

        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        if (!this->pixmap_.isNull())
        {
            if (!this->mouseOver_ && this->dimPixmap_ && this->enabled_)
            {
                painter.setOpacity(this->getCurrentDimAmount());
            }

            QRect rect = this->rect();
            int s = this->enableMargin_ ? int(6 * this->scale()) : 0;

            rect.moveLeft(s);
            rect.setRight(rect.right() - s - s);
            rect.moveTop(s);
            rect.setBottom(rect.bottom() - s - s);

            QPixmap scaled = [&]() {
                if (this->pixmapScaled_.isNull() ||
                    this->pixmapScaled_.size() != rect.size())
                {
                    ab::BenchmarkGuard benchmark("scaled image");

                    return this->pixmapScaled_ = this->pixmap_.scaled(
                               rect.size(), Qt::IgnoreAspectRatio,
                               Qt::SmoothTransformation);
                }
                else
                {
                    return this->pixmapScaled_;
                }
            }();

            painter.drawPixmap(rect, scaled);

            painter.setOpacity(1);
        }

        this->fancyPaint(painter);
    }

    void FlatButton::fancyPaint(QPainter& painter)
    {
        if (!this->enabled_)
            return;

        if (!this->palette().foreground().color().isValid())
            return;

        if (this->palette().foreground().color().alpha() == 0)
            return;

        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        painter.setRenderHint(QPainter::Antialiasing);

        QColor c = this->palette().foreground().color();

        if (this->hover_ && this->hoverMultiplier_ > 0)
        {
            QRadialGradient gradient(QPointF(mousePos_), this->width() / 2);

            gradient.setColorAt(0, QColor(c.red(), c.green(), c.blue(),
                                       int(50 * this->hoverMultiplier_)));
            gradient.setColorAt(1, QColor(c.red(), c.green(), c.blue(),
                                       int(40 * this->hoverMultiplier_)));

            painter.fillRect(this->rect(), gradient);
        }

        if (this->ripple_)
        {
            for (auto effect : this->clickEffects_)
            {
                QRadialGradient gradient(effect.position.x(),
                    effect.position.y(), effect.progress * qreal(width()) * 2,
                    effect.position.x(), effect.position.y());

                gradient.setColorAt(0, QColor(c.red(), c.green(), c.blue(),
                                           int((1 - effect.progress) * 95)));
                gradient.setColorAt(
                    0.9999, QColor(c.red(), c.green(), c.blue(),
                                int((1 - effect.progress) * 95)));
                gradient.setColorAt(
                    1, QColor(c.red(), c.green(), c.blue(), int(0)));

                painter.fillRect(this->rect(), gradient);
            }
        }
    }

    void FlatButton::enterEvent(QEvent*)
    {
        this->mouseOver_ = true;
    }

    void FlatButton::leaveEvent(QEvent*)
    {
        this->mouseOver_ = false;
    }

    void FlatButton::mousePressEvent(QMouseEvent* event)
    {
        if (!this->enabled_)
            return;

        if (event->button() != Qt::LeftButton)
            return;

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

    void FlatButton::mouseReleaseEvent(QMouseEvent* event)
    {
        if (!this->enabled_)
            return;

        if (!this->rect().contains(event->pos()))
            return;

        if (event->button() == Qt::LeftButton)
        {
            this->mouseDown_ = false;

            if (this->rect().contains(event->pos()))
                emit leftClicked();
        }

        emit clicked(event->button());
    }

    void FlatButton::mouseMoveEvent(QMouseEvent* event)
    {
        if (this->enabled_)
        {
            this->mousePos_ = event->pos();

            this->update();
        }
    }

    void FlatButton::onMouseEffectTimeout()
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
                it->progress +=
                    (mouseDown_ && &this->clickEffects_.back() == &*it) ? 0.02
                                                                        : 0.07;

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

    void FlatButton::showMenu()
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
}  // namespace ab
