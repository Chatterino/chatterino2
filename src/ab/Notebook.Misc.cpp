#include "Notebook.Misc.hpp"

#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"

#include <QLabel>

namespace
{
    int getWidth(QSize size, ab::TabOrientation orientation)
    {
        if (orientation == ab::TabOrientation::Horizontal)
            return size.width();
        else
            return size.height();
    }

    int getHeight(QSize size, ab::TabOrientation orientation)
    {
        if (orientation == ab::TabOrientation::Horizontal)
            return size.height();
        else
            return size.width();
    }

    QPoint getPoint(int x, int y, ab::TabOrientation orientation)
    {
        if (orientation == ab::TabOrientation::Horizontal)
            return {x, y};
        else
            return {y, x};
    }

    QSize getSize(int width, int height, ab::TabOrientation orientation)
    {
        if (orientation == ab::TabOrientation::Horizontal)
            return {width, height};
        else
            return {height, width};
    }
}  // namespace

namespace ab
{
    // TAB
    BasicTab::BasicTab()
    {
        this->setLayout(makeLayout<Row>([&](auto x) { this->row_ = x; },
            {
                makeWidget<QLabel>([&](auto x) { this->label_ = x; }),
            }));
    }

    QLabel& BasicTab::label()
    {
        return *this->label_;
    }

    Row& BasicTab::row()
    {
        return *this->row_;
    }

    // NOTEBOOKBUTTON
    void NotebookButton::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        this->paint(painter);

        auto rect = this->contentsRect();

        painter.fillRect(QRect(middle(rect.topLeft(), rect.bottomLeft()),
                             middle(rect.topRight(), rect.bottomRight())),
            this->palette().foreground());
        painter.fillRect(QRect(middle(rect.topLeft(), rect.topRight()),
                             middle(rect.bottomLeft(), rect.bottomRight())),
            this->palette().foreground());
    }

    // TABLAYOUT
    TabLayout::TabLayout(QWidget* parent)
        : QLayout(parent)
    {
        this->setContentsMargins(0, 0, 0, 0);
        this->setSpacing(0);
    }

    TabLayout::~TabLayout()
    {
        for (auto&& item : this->items_)
            delete item;
    }

    void TabLayout::setOrientation(TabOrientation value)
    {
        this->orientation_ = value;

        this->calcSize(true);
    }

    TabOrientation TabLayout::orientation() const
    {
        return this->orientation_;
    }

    void TabLayout::move(int from, int to)
    {
        this->items_.insert(to, this->items_.takeAt(from));

        this->calcSize(true);
    }

    // OVERRIDE
    void TabLayout::addItem(QLayoutItem* item)
    {
        this->items_.append(item);

        this->calcSize(true);
    }

    void TabLayout::insertWidget(int index, QWidget* item)
    {
        this->items_.insert(index, new QWidgetItem(item));
        this->addChildWidget(item);

        this->calcSize(true);
    }

    bool TabLayout::hasHeightForWidth() const
    {
        return false;
    }

    int TabLayout::count() const
    {
        return this->items_.count();
    }

    QLayoutItem* TabLayout::itemAt(int index) const
    {
        if (this->count() >= index || index < 0)
            return nullptr;
        else
            return this->items_.at(index);
    }

    QSize TabLayout::minimumSize() const
    {
        if (this->orientation() == TabOrientation::Horizontal)
            return QSize();
        else
            return QSize(this->calcSize().width(), 10);
    }

    QRect TabLayout::geometry() const
    {
        return this->geometry_;
    }

    void TabLayout::setGeometry(const QRect& rect)
    {
        if (this->geometry_ != rect)
        {
            this->geometry_ = rect;

            calcSize(true);
        }
    }

    QSize TabLayout::sizeHint() const
    {
        return this->calcSize();
    }

    QLayoutItem* TabLayout::takeAt(int index)
    {
        auto item = this->items_.takeAt(index);
        if (item->widget())
            item->widget()->setParent(nullptr);
        else if (item->layout())
            item->layout()->setParent(nullptr);

        this->calcSize(true);
        return item;
    }

    QSize TabLayout::calcSize(bool setGeometry) const
    {
        return this->calcSize(
            this->geometry_.width(), this->geometry_.height(), setGeometry);
    }

    QSize TabLayout::calcSize(int width, int height, bool setGeometry) const
    {
        // TODO: remove this
        setGeometry = true;

        auto x = 0;
        auto y = 0;
        auto maxHeightInLine = 0;

        for (auto&& item : this->items_)
        {
            // try figure out the size of the object
            QSize itemSize = item->sizeHint();
            if (itemSize.isEmpty())
                itemSize = item->minimumSize();
            if (itemSize.isEmpty())
                itemSize = {20, 20};

            // in current line
            if (x + getWidth(itemSize, this->orientation()) >
                getWidth({width, height}, this->orientation()))
            {
                y += maxHeightInLine;
                maxHeightInLine = 0;
                x = 0;
            }

            // set the geometry of the child
            if (setGeometry)
            {
                item->setGeometry(QRect(getPoint(x, y, this->orientation()) +
                                            this->geometry_.topLeft(),
                    itemSize));
            }

            // misc
            x += getWidth(itemSize, this->orientation());

            maxHeightInLine = std::max(
                maxHeightInLine, getHeight(itemSize, this->orientation()));
        }

        y += maxHeightInLine;

        if (setGeometry && this->height_ != y)
        {
            const_cast<TabLayout*>(this)->height_ = y;
            const_cast<TabLayout*>(this)->invalidate();
        }

        return getSize(x, y, this->orientation());
    }
}  // namespace ab
