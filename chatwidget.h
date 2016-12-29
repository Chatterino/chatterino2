#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>

#include "QFont"
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    ChatWidget(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    QFont font;
};

#endif // CHATWIDGET_H
