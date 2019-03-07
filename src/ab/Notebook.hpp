#pragma once

#include <QLayout>

#include "ab/BaseWidget.hpp"
#include "ab/FlatButton.hpp"
#include "ab/Notebook.Misc.hpp"

namespace ab
{
    class FunctionEventFilter;

    enum class NotebookOrientation { Left, Top, Right, Bottom };

    class Notebook : public BaseWidget
    {
        Q_OBJECT

    public:
        struct Item
        {
            QWidget* tab{};
            QWidget* content{};
            bool atStart{};
            bool atEnd{};
        };

        Notebook();

        void addTab(const QString& title, QWidget* page);
        void addTab(QWidget* tab, QWidget* page);
        void addTabAtStart(QWidget* tab, QWidget* page = nullptr);
        void addTabAtEnd(QWidget* tab, QWidget* page = nullptr);

        QWidget* selectedPage() const;

        void setOrientation(NotebookOrientation);
        NotebookOrientation orientation() const;

        [[deprecated]] void setUndockHandler(
            std::function<void(QWidget*, QWidget*)>);  // TODO: remove

        // opterations below work when passing a tab or a content widget
        void select(QWidget*);
        int find(QWidget*);
        void remove(QWidget*);
        void move(QWidget* from, QWidget* to);
        bool moveable(QWidget*);

        int size() const;
        Item itemAt(int index) const;
        QVector<Item>::ConstIterator begin() const;
        QVector<Item>::ConstIterator end() const;

    private:
        void handleTabEvent(QWidget* tab, QEvent*);

        void afterAdd(QWidget* tab, QWidget* content);
        void undock(QWidget* tab);

        // std::function<void(QWidget*, QWidget*)> undockHandler_;
        TabLayout* tabLayout_{};
        QBoxLayout* contentLayout_{};
        QBoxLayout* outerLayout_{};
        FunctionEventFilter* tabFilter_{};
        Item selected_{};
        NotebookOrientation orientation_;

        QVector<Item> items_;
    };
}  // namespace ab
