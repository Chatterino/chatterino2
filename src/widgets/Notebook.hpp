#pragma once

#include "pajlada/signals/signal.hpp"
#include "widgets/BaseWidget.hpp"

#include <QList>
#include <QMessageBox>
#include <QWidget>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class Window;
class UpdateDialog;
class NotebookButton;
class NotebookTab;
class SplitContainer;

enum NotebookTabDirection { Horizontal = 0, Vertical = 1 };

class Notebook : public BaseWidget
{
    Q_OBJECT

public:
    explicit Notebook(QWidget *parent);
    ~Notebook() override = default;

    NotebookTab *addPage(QWidget *page, QString title = QString(),
                         bool select = false);
    void removePage(QWidget *page);
    void removeCurrentPage();

    int indexOf(QWidget *page) const;
    virtual void select(QWidget *page);
    void selectIndex(int index);
    void selectNextTab();
    void selectPreviousTab();
    void selectLastTab();

    int getPageCount() const;
    QWidget *getPageAt(int index) const;
    int getSelectedIndex() const;
    QWidget *getSelectedPage() const;

    QWidget *tabAt(QPoint point, int &index, int maxWidth = 2000000000);
    void rearrangePage(QWidget *page, int index);

    bool getAllowUserTabManagement() const;
    void setAllowUserTabManagement(bool value);

    bool getShowAddButton() const;
    void setShowAddButton(bool value);

    void performLayout(bool animate = false);

    void setTabDirection(NotebookTabDirection direction);

protected:
    virtual void scaleChangedEvent(float scale_) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void paintEvent(QPaintEvent *) override;

    NotebookButton *getAddButton();
    NotebookButton *addCustomButton();

private:
    struct Item {
        NotebookTab *tab{};
        QWidget *page{};
        QWidget *selectedWidget{};
    };

    bool containsPage(QWidget *page);
    Item &findItem(QWidget *page);

    static bool containsChild(const QObject *obj, const QObject *child);
    NotebookTab *getTabFromPage(QWidget *page);

    QList<Item> items_;
    QWidget *selectedPage_ = nullptr;

    NotebookButton *addButton_;
    std::vector<NotebookButton *> customButtons_;

    bool allowUserTabManagement_ = false;
    bool showAddButton_ = false;
    int lineOffset_ = 20;
    NotebookTabDirection tabDirection_ = NotebookTabDirection::Horizontal;
};

class SplitNotebook : public Notebook, pajlada::Signals::SignalHolder
{
public:
    SplitNotebook(Window *parent);

    SplitContainer *addPage(bool select = false);
    SplitContainer *getOrAddSelectedPage();
    void select(QWidget *page) override;

private:
    void addCustomButtons();

    pajlada::Signals::SignalHolder signalHolder_;

    std::vector<pajlada::Signals::ScopedConnection> connections_;
};

}  // namespace chatterino
