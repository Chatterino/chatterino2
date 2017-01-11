#ifndef NOTEBOOKBUTTON_H
#define NOTEBOOKBUTTON_H

#include <QWidget>

class NotebookButton : public QWidget
{
    Q_OBJECT
public:
    static const int IconPlus = 0;
    static const int IconUser = 1;
    static const int IconSettings = 2;

    int icon = 0;

    NotebookButton(QWidget *parent);

    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

signals:
    void clicked();

private:
    bool mouseOver = false;
    bool mouseDown = false;
};

#endif  // NOTEBOOKBUTTON_H
