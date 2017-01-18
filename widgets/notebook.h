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
    Notebook(QWidget *parent);

    NotebookPage *addPage();

    enum HighlightType { none, highlighted, newMessage };

    void select(NotebookPage *page);

    NotebookPage *
    getSelectedPage()
    {
        return selectedPage;
    }

    void performLayout();

protected:
    void resizeEvent(QResizeEvent *);

    void settingsButtonMouseReleased(QMouseEvent *event);

public slots:
    void settingsButtonClicked();

private:
    QList<NotebookPage *> pages;

    NotebookButton addButton;
    NotebookButton settingsButton;
    NotebookButton userButton;

    NotebookPage *selectedPage = nullptr;
};
}
}

#endif  // NOTEBOOK_H
