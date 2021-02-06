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
    pajlada::Signals::Signal<const QMimeData *> imagePasted;

    void setCompleter(QCompleter *c);
    QCompleter *getCompleter() const;

protected:
    int heightForWidth(int) const override;
    void keyPressEvent(QKeyEvent *event) override;

    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;

private:
    // hadSpace is set to true in case the "textUnderCursor" word was after a
    // space
    QString textUnderCursor(bool *hadSpace = nullptr) const;

    QCompleter *completer_ = nullptr;
    bool completionInProgress_ = false;

    bool eventFilter(QObject *widget, QEvent *event) override;
private slots:
    void insertCompletion(const QString &completion);
};

}  // namespace chatterino
