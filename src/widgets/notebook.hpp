#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/notebookbutton.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/splitcontainer.hpp"

#include <QList>
#include <QMessageBox>
#include <QWidget>

namespace chatterino {
namespace widgets {

class Window;

class Notebook : public BaseWidget
{
    Q_OBJECT

public:
    explicit Notebook(QWidget *parent);

    NotebookTab *addPage(QWidget *page, QString title = QString(), bool select = false);
    void removePage(QWidget *page);
    void removeCurrentPage();

    int indexOf(QWidget *page) const;
    void select(QWidget *page);
    void selectIndex(int index);
    void selectNextTab();
    void selectPreviousTab();

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

    void performLayout(bool animate = true);

protected:
    virtual void scaleChangedEvent(float scale) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void paintEvent(QPaintEvent *) override;

    NotebookButton *getAddButton();
    NotebookButton *addCustomButton();

private:
    struct Item {
        NotebookTab *tab;
        QWidget *page;
    };

    QList<Item> items;
    QWidget *selectedPage = nullptr;

    NotebookButton addButton;
    std::vector<NotebookButton *> customButtons;

    bool allowUserTabManagement = false;
    bool showAddButton = false;
    int lineY = 20;

    NotebookTab *getTabFromPage(QWidget *page);
};

class SplitNotebook : public Notebook
{
public:
    SplitNotebook(Window *parent);

    SplitContainer *addPage(bool select = false);
    SplitContainer *getOrAddSelectedPage();
};

}  // namespace widgets
}  // namespace chatterino
