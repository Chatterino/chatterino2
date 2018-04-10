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
    enum HighlightType { none, highlighted, newMessage };

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
