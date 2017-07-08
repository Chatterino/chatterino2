#include "widgets/resizingtextedit.hpp"

ResizingTextEdit::ResizingTextEdit()
    : keyPressed()
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
    this->setSizePolicy(sizePolicy);

    QObject::connect(this, &QTextEdit::textChanged, this, &QWidget::updateGeometry);
}

QSize ResizingTextEdit::sizeHint() const
{
    return QSize(this->width(), this->heightForWidth(this->width()));
}

bool ResizingTextEdit::hasHeightForWidth() const
{
    return true;
}

int ResizingTextEdit::heightForWidth(int) const
{
    auto margins = this->contentsMargins();

    return margins.top() + document()->size().height() + margins.bottom() + 5;
}

QString ResizingTextEdit::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void ResizingTextEdit::keyPressEvent(QKeyEvent *event)
{
    event->ignore();

    keyPressed(event);

    if (event->key() == Qt::Key_Tab) {
        QString currentCompletionPrefix = this->textUnderCursor();
        if (!currentCompletionPrefix.size()) {
            return;
        }
        if (!this->nextCompletion) {
            // first selection
            this->completer->setCompletionPrefix(currentCompletionPrefix);
            this->nextCompletion = true;
            this->completer->complete();
            return;
        }

        // scrolling through selections
        if (!this->completer->setCurrentRow(this->completer->currentRow() + 1)) {
            // wrap over and start again
            this->completer->setCurrentRow(0);
        }
        this->completer->complete();
        return;
    }
    // (hemirt)
    // this resets the selection in the completion list, it should probably only trigger on actual
    // chat input (space, character) and not on every key input (pressing alt for example)
    this->nextCompletion = false;

    if (!event->isAccepted()) {
        QTextEdit::keyPressEvent(event);
    }
}

void ResizingTextEdit::setCompleter(QCompleter *c)
{
    if (this->completer) {
        QObject::disconnect(this->completer, 0, this, 0);
    }

    this->completer = c;

    if (!this->completer) {
        return;
    }

    this->completer->setWidget(this);
    this->completer->setCompletionMode(QCompleter::InlineCompletion);
    this->completer->setCaseSensitivity(Qt::CaseInsensitive);
    /*QObject::connect(this->completer, SIGNAL(highlighted(QString)), this,
                     SLOT(insertCompletion(QString)));
*/
    QObject::connect(completer,
                     static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::highlighted),
                     this, &ResizingTextEdit::insertCompletion);
}

void ResizingTextEdit::insertCompletion(const QString &completion)
{
    if (this->completer->widget() != this) {
        return;
    }

    QTextCursor tc = this->textCursor();
    tc.select(QTextCursor::SelectionType::WordUnderCursor);
    tc.insertText(completion);
    this->setTextCursor(tc);
}

QCompleter *ResizingTextEdit::getCompleter() const
{
    return this->completer;
}
