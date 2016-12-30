#ifndef NOTEBOOKPAGE_H
#define NOTEBOOKPAGE_H

#include "QWidget"

class NotebookTab;

class NotebookPage : public QWidget
{
Q_OBJECT

public:
    NotebookPage(QWidget *parent, NotebookTab *tab);
    NotebookTab* tab;
};

#endif // NOTEBOOKPAGE_H
