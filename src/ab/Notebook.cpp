#include "Notebook.hpp"

#include <QDebug>
#include <QLabel>

#include "ab/Column.hpp"
#include "ab/Row.hpp"
#include "ab/util/FunctionEventFilter.hpp"
#include "ab/util/MakeWidget.hpp"

#include "ab/BaseWindow.hpp"
#include "ab/util/FunctionEventFilter.hpp"
#include "ab/util/WindowsHelper.hpp"

namespace ab
{
    Notebook::Notebook()
    {
        this->setLayout(makeLayout<Column>(
            [&](auto x) { this->outerLayout_ = x; },
            {
                makeWidget<TabLayout>([&](auto x) { this->tabLayout_ = x; }),
                makeWidget<NotebookContent>([&](auto x) {
                    QSizePolicy policy(
                        QSizePolicy::Minimum, QSizePolicy::Minimum);
                    policy.setVerticalStretch(1);
                    policy.setHorizontalStretch(1);
                    x->setSizePolicy(policy);
                    auto row = new Column();
                    this->contentLayout_ = row;
                    x->setLayout(row);
                }),
            }));

        this->tabFilter_ =
            new FunctionEventFilter(this, [this](QObject* obj, QEvent* event) {
                auto w = dynamic_cast<QWidget*>(obj);

                if (w)
                    this->handleTabEvent(w, event);

                return false;
            });

        this->setOrientation(NotebookOrientation::Top);

        assert(this->tabLayout_);
        assert(this->contentLayout_);
        assert(this->outerLayout_);
    }

    void Notebook::handleTabEvent(QWidget* tab, QEvent* event)
    {
        switch (event->type())
        {
            case QEvent::MouseButtonPress:
            {
                auto e = static_cast<QMouseEvent*>(event);
                if (e->button() == Qt::LeftButton)
                {
                    this->select(tab);
                }
            }
            break;
            case QEvent::Close:
            {
                this->remove(tab);
            }
            break;
            case QEvent::MouseMove:
            {
                if (!this->moveable(tab))
                    return;

                auto e = static_cast<QMouseEvent*>(event);
                auto p = this->mapFromGlobal(e->globalPos());
                auto c = this->childAt(p);

                while (c != nullptr && find(c) == -1 && c != this)
                    c = dynamic_cast<QWidget*>(c->parent());

                //                if (this->undockHandler_ &&
                //                    e->y() >
                //                    this->tabLayout_->geometry().height() +
                //                    10)
                //                {
                //                    this->undock(tab);

                //                    return;
                //                }

                if (this->moveable(c) && c != tab &&
                    c->mapFromGlobal(e->globalPos()).x() < tab->width())
                {
                    this->move(tab, c);
                }
            }
            break;
            default:;
        }
    }

    void Notebook::addTab(const QString& title, QWidget* page)
    {
        // adds a 'BasicTab' with title
        auto tab = new BasicTab();
        tab->label().setTextFormat(Qt::PlainText);
        tab->label().setText(title);

        this->addTab(tab, page);
    }

    void Notebook::addTab(QWidget* tab, QWidget* page)
    {
        // find index to insert
        auto index = this->items_.size() - 1;

        while (index != -1 && this->items_[index].atEnd)
            index--;

        // insert item
        this->items_.insert(index + 1, {tab, page});

        // insert tab
        this->tabLayout_->insertWidget(index + 1, tab);

        // init tab
        this->afterAdd(tab, page);
    }

    void Notebook::addTabAtStart(QWidget* tab, QWidget* page)
    {
        // insert item at start
        this->items_.insert(0, {tab, page, true});

        // insert tab
        this->tabLayout_->insertWidget(0, tab);

        // init tab
        this->afterAdd(tab, page);
    }

    void Notebook::addTabAtEnd(QWidget* tab, QWidget* page)
    {
        // append item
        this->items_.append({tab, page, false, true});

        // append tab
        this->tabLayout_->addWidget(tab);

        // init tab
        this->afterAdd(tab, page);
    }

    void Notebook::afterAdd(QWidget* tab, QWidget* /*content*/)
    {
        // add click listener
        if (this->tabFilter_)
            tab->installEventFilter(this->tabFilter_);

        // select if nothing selected
        if (!this->selected_.tab)
            select(tab);
    }

    void Notebook::select(QWidget* widget)
    {
        auto index = this->find(widget);
        assert(index != -1);

        auto&& item = this->items_[index];

        // only add if the tab has a content widget
        if (item.content == nullptr)
            return;

        // remove current 'selected' effect
        for (auto&& item : this->items_)
        {
            if (item.tab->property("selected").isValid())
            {
                setPropertyAndPolish(item.tab, "selected", QVariant());
                polishChildren(item.tab);
            }
        }

        // add new 'selected' effect
        setPropertyAndPolish(item.tab, "selected", true);
        polishChildren(item.tab);

        // remove current widget
        if (this->contentLayout_->count() == 1)
        {
            auto a = this->contentLayout_->takeAt(0);
            a->widget()->hide();
            delete a;
        }

        // add new widget
        this->contentLayout_->addWidget(item.content, 1);
        item.content->setVisible(true);
        this->selected_ = item;
    }

    int Notebook::find(QWidget* widget)
    {
        // return item index or -1 if not found
        auto i = 0;

        for (auto&& item : this->items_)
        {
            if (item.content == widget || item.tab == widget)
                return i;

            i++;
        }

        return -1;
    }

    void Notebook::remove(QWidget* widget)
    {
        auto index = this->find(widget);
        assert(index != -1);

        // remove from tab layout
        delete this->tabLayout_->takeAt(index);

        // remove from list and delete
        auto item = this->items_.takeAt(index);
        item.content->deleteLater();
        item.tab->deleteLater();

        // select next tab
        [&]() {
            for (auto i = index; i < this->items_.size(); i++)
            {
                if (auto&& item = this->items_[i]; !item.atEnd && !item.atStart)
                {
                    this->select(item.tab);
                    return;
                }
            }
            for (auto i = index; i >= 0; i--)
            {
                if (auto&& item = this->items_[i]; !item.atEnd && !item.atStart)
                {
                    this->select(item.tab);
                    return;
                }
            }
        }();
    }

    void Notebook::move(QWidget* from, QWidget* to)
    {
        auto fromIndex = this->find(from);
        auto toIndex = this->find(to);
        assert(fromIndex != -1);
        assert(toIndex != -1);

        this->items_.insert(toIndex, this->items_.takeAt(fromIndex));

        // move in tab layout
        this->tabLayout_->move(fromIndex, toIndex);
    }

    void Notebook::undock(QWidget* tab)
    {
        //#ifdef USEWINSDK
        //        auto&& item = this->items_[this->find(tab)];

        //        auto w = new BaseWindow(
        //            BaseWindow::Flags(BaseWindow::Flags::EnableCustomFrame |
        //                              BaseWindow::Flags::Dragging));

        //        w->setCenterWidget(item.content);
        //        item.content = nullptr;

        //        this->remove(tab);
        //        w->show();
        //#endif
    }

    bool Notebook::moveable(QWidget* widget)
    {
        auto index = this->find(widget);

        return index != -1 && !this->items_[index].atEnd &&
               !this->items_[index].atStart;
    }

    int Notebook::count() const
    {
        return this->items_.size();
    }

    int Notebook::regularCount() const
    {
        return std::accumulate(this->items_.begin(), this->items_.end(), 0,
            [](auto val, const Item& element) {
                return val + (element.atStart || element.atEnd ? 0 : 1);
            });
    }

    Notebook::Item Notebook::itemAt(int index) const
    {
        return this->items_.at(index);
    }

    QVector<Notebook::Item>::ConstIterator Notebook::begin() const
    {
        return this->items_.cbegin();
    }

    QVector<Notebook::Item>::ConstIterator Notebook::end() const
    {
        return this->items_.cend();
    }

    QWidget* Notebook::selectedPage() const
    {
        return this->selected_.content;
    }

    void Notebook::setOrientation(NotebookOrientation value)
    {
        bool isVertical = value == NotebookOrientation::Top ||
                          value == NotebookOrientation::Bottom;

        this->tabLayout_->setOrientation(
            isVertical ? TabOrientation::Horizontal : TabOrientation::Vertical);

        if (value == NotebookOrientation::Left)
            this->outerLayout_->setDirection(QBoxLayout::LeftToRight);
        else if (value == NotebookOrientation::Top)
            this->outerLayout_->setDirection(QBoxLayout::TopToBottom);
        else if (value == NotebookOrientation::Right)
            this->outerLayout_->setDirection(QBoxLayout::RightToLeft);
        else if (value == NotebookOrientation::Bottom)
            this->outerLayout_->setDirection(QBoxLayout::BottomToTop);

        //        if (isVertical)
        //        {
        //            this->setLayout(this->column_.get());

        //            if (value == NotebookOrientation::Top)
        //                this->column_->addLayout(this->tabLayout_);
        //            this->column_->addWidget(this->notebookContent_);
        //            if (value != NotebookOrientation::Top)
        //                this->column_->addLayout(this->tabLayout_);
        //        }
        //        else
        //        {
        //            auto row = new Row();

        //            if (value == NotebookOrientation::Left)
        //                this->row_->addLayout(this->tabLayout_);
        //            this->row_->addWidget(this->notebookContent_);
        //            if (value != NotebookOrientation::Left)
        //                this->row_->addLayout(this->tabLayout_);
        //        }
    }

    NotebookOrientation Notebook::orientation() const
    {
        return this->orientation_;
    }

    void Notebook::setUndockHandler(
        std::function<void(QWidget*, QWidget*)> handler)
    {
        //        this->undockHandler_ = std::move(handler);
    }
}  // namespace ab
