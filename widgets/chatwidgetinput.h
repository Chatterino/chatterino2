#ifndef CHATWIDGETINPUT_H
#define CHATWIDGETINPUT_H

#include "resizingtextedit.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTextEdit>
#include <QVBoxLayout>
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
    QHBoxLayout hbox;
    ResizingTextEdit edit;
};
}
}

#endif  // CHATWIDGETINPUT_H
