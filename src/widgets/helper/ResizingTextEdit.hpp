#pragma once

#include <QCompleter>
#include <QKeyEvent>
#include <QTextEdit>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit();

    QSize sizeHint() const override;

    bool hasHeightForWidth() const override;

    pajlada::Signals::Signal<QKeyEvent *> keyPressed;
    pajlada::Signals::NoArgSignal focused;
    pajlada::Signals::NoArgSignal focusLost;

    void setCompleter(QCompleter *c);
    QCompleter *getCompleter() const;

protected:
    int heightForWidth(int) const override;
    void keyPressEvent(QKeyEvent *event) override;

    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QCompleter *completer = nullptr;
    bool completionInProgress = false;

    // hadSpace is set to true in case the "textUnderCursor" word was after a space
    QString textUnderCursor(bool *hadSpace = nullptr) const;

private slots:
    void insertCompletion(const QString &completion);
};

}
