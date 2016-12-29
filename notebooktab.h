#ifndef NOTEBOOKTAB_H
#define NOTEBOOKTAB_H

#include "QObject"
#include "notebookpage.h"

class Notebook;

class NotebookTab : public QWidget
{
Q_OBJECT

public:
    NotebookTab(QWidget *parent, Notebook *notebook, NotebookPage *page);

private:
    Notebook *notebook;
    NotebookPage *page;
};

#endif // NOTEBOOKTAB_H
