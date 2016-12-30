#ifndef NOTEBOOKPAGE_H
#define NOTEBOOKPAGE_H

#include "QWidget"
#include "notebookpage.h"
#include "notebooktab.h"

class NotebookPage : public QWidget
{
Q_OBJECT

public:
    NotebookPage(NotebookTab *tab);
    NotebookTab* tab;

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
};

#endif // NOTEBOOKPAGE_H
