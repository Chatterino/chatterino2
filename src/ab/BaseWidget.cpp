#include "ab/BaseWidget.hpp"

//#include "BaseSettings.hpp"
//#include "BaseTheme.hpp"
//#include "util/Log.hpp"
#include "ab/BaseWindow.hpp"

#include <QChildEvent>
#include <QDebug>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QtGlobal>

namespace ab
{
    void setPropertyAndPolish(
        QWidget* widget, const char* name, const QVariant& value)
    {
        widget->setProperty(name, value);
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }

    void polishChildren(QWidget* widget)
    {
        for (auto&& child : widget->findChildren<QWidget*>())
        {
            child->style()->unpolish(child);
            child->style()->polish(child);
        }
    }

    QPoint middle(const QPoint& a, const QPoint& b)
    {
        return {(a.x() + b.x()) / 2, (a.y() + b.y()) / 2};
    }

    BaseWidget::BaseWidget(QWidget* parent, Qt::WindowFlags f)
        : QFrame(parent, f)
    {
    }

    float BaseWidget::scale() const
    {
        //    if (this->overrideScale_)
        //    {
        //        return this->overrideScale_.get();
        //    }
        //    else if (auto baseWidget =
        //    dynamic_cast<BaseWidget*>(this->window()))
        //    {
        //        return baseWidget->scale_;
        //    }
        //    else
        //    {
        return 1.f;
        //    }
    }

    // void BaseWidget::setScale(float value)
    //{
    //    // update scale value
    //    this->scale_ = value;

    //    this->scaleChangedEvent(this->scale());
    //    this->scaleChanged.invoke(this->scale());

    //    this->setScaleIndependantSize(this->scaleIndependantSize());
    //}

    // void BaseWidget::setOverrideScale(boost::optional<float> value)
    //{
    //    this->overrideScale_ = value;
    //    this->setScale(this->scale());
    //}

    // boost::optional<float> BaseWidget::overrideScale() const
    //{
    //    return this->overrideScale_;
    //}

    QSize BaseWidget::scaleIndependantSize() const
    {
        return this->scaleIndependantSize_;
    }

    int BaseWidget::scaleIndependantWidth() const
    {
        return this->scaleIndependantSize_.width();
    }

    int BaseWidget::scaleIndependantHeight() const
    {
        return this->scaleIndependantSize_.height();
    }

    void BaseWidget::setScaleIndependantSize(int width, int height)
    {
        this->setScaleIndependantSize(QSize(width, height));
    }

    void BaseWidget::setScaleIndependantSize(QSize size)
    {
        this->scaleIndependantSize_ = size;

        if (size.width() > 0)
        {
            this->setFixedWidth(int(size.width() * this->scale()));
        }
        if (size.height() > 0)
        {
            this->setFixedHeight(int(size.height() * this->scale()));
        }
    }

    void BaseWidget::setScaleIndependantWidth(int value)
    {
        this->setScaleIndependantSize(
            QSize(value, this->scaleIndependantSize_.height()));
    }

    void BaseWidget::setScaleIndependantHeight(int value)
    {
        this->setScaleIndependantSize(
            QSize(this->scaleIndependantSize_.width(), value));
    }

    float BaseWidget::qtFontScale() const
    {
        if (auto window = dynamic_cast<BaseWindow*>(this->window()))
        {
            return this->scale() / window->nativeScale_;
        }
        else
        {
            return this->scale();
        }
    }

    void BaseWidget::childEvent(QChildEvent* event)
    {
        if (event->added())
        {
            // add element if it's a basewidget
            if (auto widget = dynamic_cast<BaseWidget*>(event->child()))
            {
                if (this->theme_)
                {
                    widget->themeChangedEvent(*this->theme_);
                }
                this->widgets_.push_back(widget);
            }
        }
        else if (event->removed())
        {
            // find element to be removed
            auto it = std::find_if(this->widgets_.begin(), this->widgets_.end(),
                [&](auto&& x) { return x == event->child(); });

            // remove if found
            if (it != this->widgets_.end())
            {
                this->widgets_.erase(it);
            }
        }
    }

    void BaseWidget::showEvent(QShowEvent*)
    {
        //    this->setScale(this->scale());
        //    this->themeChangedEvent();
    }

    void BaseWidget::scaleChangedEvent(float)
    {
    }

    void BaseWidget::themeChangedEvent(Theme&)
    {
        // Do any color scheme updates here
    }

    void BaseWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);

        QStyleOption option;
        option.initFrom(this);

        this->style()->drawPrimitive(
            QStyle::PE_Widget, &option, &painter, this);
    }
}  // namespace ab
