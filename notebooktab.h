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

    NotebookPage *page;
    QString text;

    bool getSelected();
    void setSelected(bool value);

    int getHighlightStyle();
    void setHighlightStyle(int style);

    static const int HighlightNone = 0;
    static const int HighlightHighlighted = 1;
    static const int HighlightNewMessage = 2;

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;

private:
    Notebook *notebook;
    bool selected = false;

    bool mouseOver = false;
    bool mouseDown = false;
    int highlightStyle;
};

#endif  // NOTEBOOKTAB_H
