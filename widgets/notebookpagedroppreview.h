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

    void hideEvent(QHideEvent *);

    QPropertyAnimation positionAnimation;
    QRect desiredGeometry;
    bool animate;
};
}  // namespace widgets
}  // namespace chatterino

#endif  // NOTEBOOKPAGEDROPPREVIEW_H
