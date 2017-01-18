#ifndef NOTEBOOKPAGEDROPPREVIEW_H
#define NOTEBOOKPAGEDROPPREVIEW_H

#include <QPropertyAnimation>
#include <QWidget>

namespace chatterino {
namespace widgets {

class NotebookPageDropPreview : public QWidget
{
public:
    NotebookPageDropPreview(QWidget *parent);

    void setBounds(const QRect &rect);

protected:
    void paintEvent(QPaintEvent *);

    QPropertyAnimation positionAnimation;
    QRect desiredGeometry;
};
}
}

#endif  // NOTEBOOKPAGEDROPPREVIEW_H
