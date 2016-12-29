#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include <QWidget>
#include <QList>
#include "notebookpage.h"
#include "notebooktab.h"

class Notebook : public QWidget
{
Q_OBJECT

public:
    Notebook(QWidget *parent);

    NotebookPage AddPage();

private:
    QList<std::tuple<NotebookPage, NotebookTab>> pages;
};

#endif // NOTEBOOK_H
