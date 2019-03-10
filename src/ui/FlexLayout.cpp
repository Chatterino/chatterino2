#include "FlexLayout.hpp"

#include "FlexLayout.Private.hpp"

#include <QWidget>

namespace
{
    QLayoutItem* find(const QVector<QLayoutItem*>& items, QWidget* widget)
    {
        if (widget == nullptr)
            return nullptr;

        for (auto&& item : items)
            if (item->widget() == widget)
                return item;

        return nullptr;
    }
}  // namespace

namespace chatterino::ui
{
    // FlexLayout
    FlexLayout::FlexLayout()
        : root_(new FlexItem())
    {
    }

    void FlexLayout::addItem(QLayoutItem* item)
    {
        this->addItemRelativeTo(
            item, static_cast<QLayoutItem*>(nullptr), Direction::Right);
    }

    void FlexLayout::addWidgetRelativeTo(
        QWidget* widget, QWidget* relativeTo, Direction direction)
    {
        // forward
        auto item = new QWidgetItem(widget);

        this->addItemRelativeTo(item, relativeTo, direction);
    }

    void FlexLayout::addWidgetRelativeTo(
        QWidget* widget, QLayoutItem* relativeTo, Direction direction)
    {
        // forward
        auto item = new QWidgetItem(widget);

        this->addItemRelativeTo(item, relativeTo, direction);
    }

    void FlexLayout::addItemRelativeTo(
        QLayoutItem* item, QWidget* relativeTo, Direction direction)
    {
        // forward
        auto relativeToItem = find(this->items_, relativeTo);

        this->addItemRelativeTo(item, relativeToItem, direction);
    }

    void FlexLayout::addItemRelativeTo(
        QLayoutItem* item, QLayoutItem* relativeTo, Direction direction)
    {
        // insert into item tree
        this->root_->insert(item, relativeTo, direction);

        // insert into item list
        this->items_.append(item);

        // add child widget/layout
        if (item->widget())
            this->addChildWidget(item->widget());

        if (item->layout())
            this->addChildLayout(item->layout());

        this->performLayout();
        // qDebug() << this->items_.size();
        this->root_->print();
    }

    void FlexLayout::addLayout(QLayout* layout)
    {
        this->addItem(layout);
    }

    QSize FlexLayout::sizeHint() const
    {
        return QSize();
    }

    QLayoutItem* FlexLayout::itemAt(int index) const
    {
        if (index < 0 || index >= this->items_.count())
            return nullptr;
        else
            return this->items_[index];
    }

    QLayoutItem* FlexLayout::takeAt(int index)
    {
        // take from item list
        auto item = this->items_.takeAt(index);

        if (item->widget())
            item->widget()->setParent(nullptr);
        else if (item->layout())
            item->layout()->setParent(nullptr);

        // take from item tree
        this->root_->take(item);

        this->performLayout();

        // qDebug() << this->items_.size();
        // this->root_->print();

        return item;
    }

    int FlexLayout::count() const
    {
        return this->items_.count();
    }

    void FlexLayout::setGeometry(const QRect& rect)
    {
        this->geometry_ = rect;

        this->performLayout();
    }

    QRect FlexLayout::geometry() const
    {
        return this->geometry_;
    }

    void FlexLayout::invalidate()
    {
        this->performLayout();
    }

    void FlexLayout::performLayout()
    {
        this->root_->performLayout(this->geometry_);
    }
}  // namespace chatterino::ui
