#ifndef NOTEBOOKPAGEDROPPREVIEW_H
#define NOTEBOOKPAGEDROPPREVIEW_H

#include <QWidget>

class NotebookPageDropPreview : public QWidget
{
public:
    NotebookPageDropPreview(QWidget *parent);

protected:
    void paintEvent(QPaintEvent *);
};

#endif // NOTEBOOKPAGEDROPPREVIEW_H
