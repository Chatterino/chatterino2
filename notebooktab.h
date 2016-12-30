#ifndef NOTEBOOKTAB_H
#define NOTEBOOKTAB_H

#include "QWidget"

class Notebook;
class NotebookPage;

class NotebookTab : public QWidget
{
Q_OBJECT

public:
    NotebookTab(Notebook *notebook);

    void calcSize();

    NotebookPage* page;
    QString text;

    bool getSelected();
    void setSelected(bool value);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

private:
    Notebook *notebook;
    bool selected = false;

    bool mouseOver = false;
    bool mouseDown = false;
};

#endif // NOTEBOOKTAB_H
