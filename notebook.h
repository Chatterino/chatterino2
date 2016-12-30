#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include <QWidget>
#include <QList>
#include "notebookpage.h"
#include "notebooktab.h"
#include "notebookbutton.h"

class Notebook : public QWidget
{
Q_OBJECT

public:
    Notebook(QWidget *parent);

    NotebookPage* addPage();

    enum HighlightType { none, highlighted, newMessage };

protected:
    void resizeEvent(QResizeEvent *);

private:
    QList<NotebookPage*> pages;

    NotebookButton addButton;
    NotebookButton settingsButton;
    NotebookButton userButton;

    void layout();
};

#endif // NOTEBOOK_H
