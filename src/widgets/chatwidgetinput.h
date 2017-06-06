#ifndef CHATWIDGETINPUT_H
#define CHATWIDGETINPUT_H

#include "resizingtextedit.h"
#include "widgets/chatwidgetheaderbutton.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {
namespace widgets {

class ChatWidget;

class ChatWidgetInput : public QWidget
{
    Q_OBJECT

public:
    ChatWidgetInput(ChatWidget *parent);
    ~ChatWidgetInput();

protected:
    void paintEvent(QPaintEvent *);

    void resizeEvent(QResizeEvent *);

private:
    ChatWidget *_chatWidget;

    QHBoxLayout _hbox;
    QVBoxLayout _vbox;
    QHBoxLayout _editContainer;
    ResizingTextEdit _edit;
    QLabel _textLengthLabel;
    ChatWidgetHeaderButton _emotesLabel;

private slots:
    void refreshTheme();
    void setMessageLengthVisisble(bool value)
    {
        _textLengthLabel.setHidden(!value);
    }
    void editTextChanged();
    //    void editKeyPressed(QKeyEvent *event);
};
}  // namespace widgets
}  // namespace chatterino

#endif  // CHATWIDGETINPUT_H
