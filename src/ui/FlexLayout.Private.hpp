#pragma once

#include <QVector>
#include <memory>

class QObject;
class QWidget;
class QLayoutItem;
class QRect;

namespace chatterino::ui
{
    enum class Direction;
    class FlexLayout;

    class FlexItem : public std::enable_shared_from_this<FlexItem>
    {
    public:
        enum FlexItemType { Empty, Item, Column, Row };
        FlexItem();
        explicit FlexItem(QLayoutItem*);
        explicit FlexItem(QWidget*);
        explicit FlexItem(FlexItemType type);

        bool isEmpty() const;
        bool isItem() const;
        bool isColumn() const;
        bool isRow() const;

        void addChild(const std::shared_ptr<FlexItem>& widget);

        void print(int indent = 0) const;

        double flex = 1;

    private:
        std::shared_ptr<FlexItem> find(QLayoutItem*);
        // only call this on the root item
        void root_insert(
            QLayoutItem* widget, QLayoutItem* relativeTo, Direction direction);
        // only call this on the root item
        void root_add(QLayoutItem*);
        // only call this on the root item
        QLayoutItem* root_take(QLayoutItem* item);

        void performLayout(const QRect& rect);
        void addLayoutItemsRec(std::vector<QLayoutItem*>& items);

        // asd
        void clear();
        void flatten();
        std::shared_ptr<FlexItem> clone();

        bool insertRec(const std::shared_ptr<FlexItem>& item, FlexItem* parent,
            FlexItem* relativeTo, Direction direction);
        QLayoutItem* takeRec(QLayoutItem* item, FlexItem* parent);

        FlexItemType type_;
        QLayoutItem* item_;
        QVector<std::shared_ptr<FlexItem>> items_;

        friend class FlexLayout;
    };
}  // namespace chatterino::ui
