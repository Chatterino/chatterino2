#pragma once

#include <QLayout>
#include <QVector>

namespace chatterino::ui
{
    enum class Direction { Left, Above, Right, Below };

    struct FlexItem;

    class FlexLayout : public QLayout
    {
    public:
        FlexLayout();

        void addItem(QLayoutItem*) override;
        void addWidgetRelativeTo(
            QWidget* widget, QWidget* relativeTo, Direction);
        void addWidgetRelativeTo(
            QWidget* widget, QLayoutItem* relativeTo, Direction);
        void addItemRelativeTo(
            QLayoutItem* item, QWidget* relativeTo, Direction);
        void addItemRelativeTo(
            QLayoutItem* item, QLayoutItem* relativeTo, Direction);
        void addLayout(QLayout*);

        // interface
        QSize sizeHint() const override;
        QLayoutItem* itemAt(int index) const override;
        QLayoutItem* takeAt(int index) override;
        int count() const override;

        void setGeometry(const QRect&) override;
        QRect geometry() const override;
        void invalidate() override;

    private:
        void performLayout();

        std::shared_ptr<FlexItem> root_;

        QRect geometry_;
        QVector<QLayoutItem*> items_;
    };
}  // namespace chatterino::ui
