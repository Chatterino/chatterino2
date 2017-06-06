#ifndef NOTEBOOKBUTTON_H
#define NOTEBOOKBUTTON_H

#include "fancybutton.h"

#include <QWidget>

namespace chatterino {
namespace widgets {

class NotebookButton : public FancyButton
{
    Q_OBJECT

public:
    static const int IconPlus = 0;
    static const int IconUser = 1;
    static const int IconSettings = 2;

    int icon = 0;

    NotebookButton(QWidget *parent);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void clicked();

private:
    bool _mouseOver = false;
    bool _mouseDown = false;
    QPoint _mousePos;
};

}  // namespace widgets
}  // namespace chatterino

#endif  // NOTEBOOKBUTTON_H
