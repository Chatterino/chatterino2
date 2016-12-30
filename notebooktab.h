#ifndef NOTEBOOKTAB_H
#define NOTEBOOKTAB_H

#include "QObject"
#include "notebookpage.h"

class Notebook;

class NotebookTab : public QWidget
{
Q_OBJECT

public:
    NotebookTab(Notebook *notebook);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

    bool IsSelected();

private:
    Notebook *notebook;

    bool isSelected;
    bool mouseOver = false;
    bool mouseDown = false;
};

#endif // NOTEBOOKTAB_H
