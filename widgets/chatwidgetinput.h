#ifndef CHATWIDGETINPUT_H
#define CHATWIDGETINPUT_H

#include <QPaintEvent>
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
};
}
}

#endif  // CHATWIDGETINPUT_H
