#pragma once

#include "resizingtextedit.hpp"
#include "widgets/chatwidgetheaderbutton.hpp"

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
    ChatWidgetInput(ChatWidget *_chatWidget);
    ~ChatWidgetInput();

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

private:
    ChatWidget *const chatWidget;

    QHBoxLayout hbox;
    QVBoxLayout vbox;
    QHBoxLayout editContainer;
    ResizingTextEdit textInput;
    QLabel textLengthLabel;
    ChatWidgetHeaderButton emotesLabel;

private slots:
    void refreshTheme();
    void setMessageLengthVisisble(bool value)
    {
        textLengthLabel.setHidden(!value);
    }
    void editTextChanged();
    //    void editKeyPressed(QKeyEvent *event);

    friend class ChatWidget;
};

}  // namespace widgets
}  // namespace chatterino
