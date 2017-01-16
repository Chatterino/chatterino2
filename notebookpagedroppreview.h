#ifndef NOTEBOOKPAGEDROPPREVIEW_H
#define NOTEBOOKPAGEDROPPREVIEW_H

#include <QPropertyAnimation>
#include <QWidget>

class NotebookPageDropPreview : public QWidget
{
public:
    NotebookPageDropPreview(QWidget *parent);

    void setBounds(const QRect &rect);

protected:
    void paintEvent(QPaintEvent *);

    QPropertyAnimation m_positionAnimation;
    QRect m_desiredGeometry;
};

#endif  // NOTEBOOKPAGEDROPPREVIEW_H
