#pragma once

#include "emotemanager.hpp"
#include "resizingtextedit.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/emotepopup.hpp"
#include "widgets/rippleeffectlabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

class ChatWidget;

class ChatWidgetInput : public BaseWidget
{
    Q_OBJECT

public:
    ChatWidgetInput(ChatWidget *_chatWidget, EmoteManager &);
    ~ChatWidgetInput();

protected:
    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    ChatWidget *const chatWidget;
    EmotePopup *emotePopup = nullptr;
    EmoteManager &emoteManager;

    boost::signals2::connection textLengthVisibleChangedConnection;
    QHBoxLayout hbox;
    QVBoxLayout vbox;
    QHBoxLayout editContainer;
    ResizingTextEdit textInput;
    QLabel textLengthLabel;
    RippleEffectLabel emotesLabel;
    QStringList prevMsg;
    unsigned int prevIndex = 0;
    virtual void refreshTheme() override;

private slots:
    void editTextChanged();
    //    void editKeyPressed(QKeyEvent *event);

    friend class ChatWidget;
};

}  // namespace widgets
}  // namespace chatterino
