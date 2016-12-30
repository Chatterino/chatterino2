#ifndef NOTEBOOKBUTTON_H
#define NOTEBOOKBUTTON_H

#include <QWidget>

class NotebookButton : public QWidget
{
    Q_OBJECT
public:
    static const int IconPlus = 0;
    static const int IconUser = 0;
    static const int IconSettings = 0;

    int icon = 0;

    NotebookButton(QWidget *parent);

    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

private:
    bool mouseOver = false;
    bool mouseDown = false;
};

#endif // NOTEBOOKBUTTON_H
