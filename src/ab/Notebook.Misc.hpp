#pragma once

#include <QLayout>

#include "ab/BaseWidget.hpp"
#include "ab/FlatButton.hpp"

class QLabel;

namespace ab
{
    class Column;
    class Row;

    class BasicTab : public FlatButton
    {
        Q_OBJECT

    public:
        BasicTab();

        QLabel& label();

    protected:
        Row& row();

    private:
        QLabel* label_;
        Row* row_;
    };

    class NotebookButton : public FlatButton
    {
        Q_OBJECT

    protected:
        void paintEvent(QPaintEvent*) override;
    };

    class NotebookContent : public QFrame
    {
        Q_OBJECT
    };

    enum class TabOrientation { Horizontal, Vertical };

    class TabLayout : public QLayout
    {
    public:
        explicit TabLayout(QWidget* parent = nullptr);
        ~TabLayout() override;

        // methods
        void setOrientation(TabOrientation);
        TabOrientation orientation() const;
        void move(int from, int to);

        // override
        void addItem(QLayoutItem* item) override;
        void insertWidget(int index, QWidget* item);
        bool hasHeightForWidth() const override;
        int count() const override;
        QLayoutItem* itemAt(int index) const override;
        QSize minimumSize() const override;
        QRect geometry() const override;
        void setGeometry(const QRect& rect) override;
        QSize sizeHint() const override;
        QLayoutItem* takeAt(int index) override;

    private:
        // methods
        QSize calcSize(bool setGeometry = false) const;
        QSize calcSize(int width, int height, bool setGeometry = false) const;

        // variables
        QList<QLayoutItem*> items_;
        QRect geometry_;
        int height_{};
        TabOrientation orientation_ = TabOrientation::Horizontal;
    };
}  // namespace ab
