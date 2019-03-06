#pragma once

#include <QVector>
#include <memory>

#ifndef FLEXLAYOUTPRIVATE
#    define FLEXLAYOUTPRIVATE private:
#endif

class QObject;
class QWidget;
class QLayoutItem;
class QRect;

namespace chatterino::ui
{
    enum class Direction;

    class FlexItem : public std::enable_shared_from_this<FlexItem>
    {
    public:
        enum FlexItemType { Empty, Item, Column, Row };
        FlexItem();

        FLEXLAYOUTPRIVATE
        explicit FlexItem(QLayoutItem*);
        explicit FlexItem(FlexItemType type);

    public:
        bool isEmpty();
        bool isItem();
        bool isColumn();
        bool isRow();

        std::shared_ptr<FlexItem> find(QLayoutItem*);
        void insert(
            QLayoutItem* widget, QLayoutItem* relativeTo, Direction direction);
        void add(QLayoutItem*);
        QLayoutItem* take(QLayoutItem* item);

        void performLayout(const QRect& rect);
        void print(int indent = 0);

        double flex = 1;

    private:
        void clear();
        void flatten();
        std::shared_ptr<FlexItem> clone();

        bool insertRec(const std::shared_ptr<FlexItem>& item, FlexItem* parent,
            FlexItem* relativeTo, Direction direction);
        QLayoutItem* takeRec(QLayoutItem* item, FlexItem* parent);

        FlexItemType type_;
        QLayoutItem* item_;
        QVector<std::shared_ptr<FlexItem>> items_;
    };
}  // namespace chatterino::ui
