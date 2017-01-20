#ifndef CHATWIDGETINPUT_H
#define CHATWIDGETINPUT_H

#include <QPaintEvent>
#include <QTextEdit>
#include <QWidget>

namespace chatterino {
namespace widgets {

class ChatWidgetInput : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetInput();

protected:
    void paintEvent(QPaintEvent *);

private:
    QTextEdit edit;
};
}
}

#endif  // CHATWIDGETINPUT_H
