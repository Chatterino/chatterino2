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

class Notebook2 : public BaseWidget
{
    Q_OBJECT

public:
    explicit Notebook2(QWidget *parent);

    NotebookTab2 *addPage(QWidget *page, bool select = false);
    void removePage(QWidget *page);
    void removeCurrentPage();

    int indexOf(QWidget *page) const;
    void select(QWidget *page);
    void selectIndex(int index);
    void selectNextTab();
    void selectPreviousTab();

    int getPageCount() const;
    int getSelectedIndex() const;
    QWidget *getSelectedPage() const;

    QWidget *tabAt(QPoint point, int &index, int maxWidth = 2000000000);
    void rearrangePage(QWidget *page, int index);

    bool getAllowUserTabManagement() const;
    void setAllowUserTabManagement(bool value);

    bool getShowAddButton() const;
    void setShowAddButton(bool value);

protected:
    virtual void scaleChangedEvent(float scale) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void paintEvent(QPaintEvent *) override;

private:
    struct Item {
        NotebookTab2 *tab;
        QWidget *page;
    };

    QList<Item> items;
    QWidget *selectedPage = nullptr;

    NotebookButton addButton;

    bool allowUserTabManagement = false;
    bool showAddButton = false;
    int lineY = 20;

    void performLayout(bool animate = true);

    NotebookTab2 *getTabFromPage(QWidget *page);
};

class Notebook : public BaseWidget
{
    Q_OBJECT

public:
    explicit Notebook(Window *parent, bool _showButtons);

    SplitContainer *addNewPage(bool select = false);

    void removePage(SplitContainer *page);
    void removeCurrentPage();
    void select(SplitContainer *page);
    void selectIndex(int index);

    SplitContainer *getOrAddSelectedPage();
    SplitContainer *getSelectedPage();

    void performLayout(bool animate = true);

    int tabCount();
    SplitContainer *tabAt(QPoint point, int &index, int maxWidth = 2000000000);
    SplitContainer *tabAt(int index);
    void rearrangePage(SplitContainer *page, int index);

    void nextTab();
    void previousTab();

protected:
    void scaleChangedEvent(float scale);
    void resizeEvent(QResizeEvent *);

    void settingsButtonMouseReleased(QMouseEvent *event);

public slots:
    void settingsButtonClicked();
    void usersButtonClicked();
    void addPageButtonClicked();

private:
    Window *parentWindow;

    QList<SplitContainer *> pages;

    NotebookButton addButton;
    NotebookButton settingsButton;
    NotebookButton userButton;

    SplitContainer *selectedPage = nullptr;

    bool showButtons;

    QMessageBox closeConfirmDialog;
};

}  // namespace widgets
}  // namespace chatterino
