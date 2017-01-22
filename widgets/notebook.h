#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include "widgets/notebookbutton.h"
#include "widgets/notebookpage.h"
#include "widgets/notebooktab.h"

#include <QList>
#include <QWidget>

namespace chatterino {
namespace widgets {

class Notebook : public QWidget
{
    Q_OBJECT

public:
    enum HighlightType { none, highlighted, newMessage };

    Notebook(QWidget *parent);

    NotebookPage *addPage(bool select = false);

    void removePage(NotebookPage *page);
    void select(NotebookPage *page);

    NotebookPage *
    getSelectedPage()
    {
        return selectedPage;
    }

    void performLayout();

    NotebookPage *tabAt(QPoint point, int &index);
    void rearrangePage(NotebookPage *page, int index);

protected:
    void resizeEvent(QResizeEvent *);

    void settingsButtonMouseReleased(QMouseEvent *event);

public slots:
    void settingsButtonClicked();
    void usersButtonClicked();
    void addPageButtonClicked();

private:
    QList<NotebookPage *> pages;

    NotebookButton addButton;
    NotebookButton settingsButton;
    NotebookButton userButton;

    NotebookPage *selectedPage;
};
}
}

#endif  // NOTEBOOK_H
