#include "widgets/helper/ResizingTextEdit.hpp"
#include "common/Common.hpp"
#include "common/CompletionModel.hpp"

namespace chatterino {

ResizingTextEdit::ResizingTextEdit()
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
    this->setSizePolicy(sizePolicy);
    this->setAcceptRichText(false);

    QObject::connect(this, &QTextEdit::textChanged, this, &QWidget::updateGeometry);

    this->setFocusPolicy(Qt::ClickFocus);
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

QString ResizingTextEdit::textUnderCursor(bool *hadSpace) const
{
    auto currentText = this->toPlainText();

    QTextCursor tc = this->textCursor();

    auto textUpToCursor = currentText.left(tc.selectionStart());

    auto words = textUpToCursor.splitRef(' ');
    if (words.size() == 0) {
        return QString();
    }

    bool first = true;
    QString lastWord;
    for (auto it = words.crbegin(); it != words.crend(); ++it) {
        auto word = *it;

        if (first && word.isEmpty()) {
            first = false;
            if (hadSpace != nullptr) {
                *hadSpace = true;
            }
            continue;
        }

        lastWord = word.toString();
        break;
    }

    if (lastWord.isEmpty()) {
        return QString();
    }

    return lastWord;
}

void ResizingTextEdit::keyPressEvent(QKeyEvent *event)
{
    event->ignore();

    this->keyPressed.invoke(event);

    bool doComplete = (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) &&
                      (event->modifiers() & Qt::ControlModifier) == Qt::NoModifier;

    if (doComplete) {
        // check if there is a completer
        if (!this->completer) {
            return;
        }

        QString currentCompletionPrefix = this->textUnderCursor();

        // check if there is something to complete
        if (!currentCompletionPrefix.size()) {
            return;
        }

        auto *completionModel = static_cast<CompletionModel *>(this->completer->model());

        if (!this->completionInProgress) {
            // First type pressing tab after modifying a message, we refresh our completion model
            this->completer->setModel(completionModel);
            completionModel->refresh();
            this->completionInProgress = true;
            this->completer->setCompletionPrefix(currentCompletionPrefix);
            this->completer->complete();
            return;
        }

        // scrolling through selections
        if (event->key() == Qt::Key_Tab) {
            if (!this->completer->setCurrentRow(this->completer->currentRow() + 1)) {
                // wrap over and start again
                this->completer->setCurrentRow(0);
            }
        } else {
            if (!this->completer->setCurrentRow(this->completer->currentRow() - 1)) {
                // wrap over and start again
                this->completer->setCurrentRow(this->completer->completionCount() - 1);
            }
        }

        this->completer->complete();
        return;
    }

    // (hemirt)
    // this resets the selection in the completion list, it should probably only trigger on actual
    // chat input (space, character) and not on every key input (pressing alt for example)
    // (fourtf)
    // fixed for shift+tab, there might be a better solution but nobody is gonna bother anyways
    if (event->key() != Qt::Key_Shift && event->key() != Qt::Key_Control &&
        event->key() != Qt::Key_Alt && event->key() != Qt::Key_Super_L &&
        event->key() != Qt::Key_Super_R) {
        this->completionInProgress = false;
    }

    if (!event->isAccepted()) {
        QTextEdit::keyPressEvent(event);
    }
}

void ResizingTextEdit::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);

    if (event->gotFocus()) {
        this->focused.invoke();
    }
}

void ResizingTextEdit::focusOutEvent(QFocusEvent *event)
{
    QTextEdit::focusOutEvent(event);

    if (event->lostFocus()) {
        this->focusLost.invoke();
    }
}

void ResizingTextEdit::setCompleter(QCompleter *c)
{
    if (this->completer) {
        QObject::disconnect(this->completer, nullptr, this, nullptr);
    }

    this->completer = c;

    if (!this->completer) {
        return;
    }

    this->completer->setWidget(this);
    this->completer->setCompletionMode(QCompleter::InlineCompletion);
    this->completer->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(completer,
                     static_cast<void (QCompleter::*)(const QString &)>(&QCompleter::highlighted),
                     this, &ResizingTextEdit::insertCompletion);
}

void ResizingTextEdit::insertCompletion(const QString &completion)
{
    if (this->completer->widget() != this) {
        return;
    }

    bool hadSpace = false;
    auto prefix = this->textUnderCursor(&hadSpace);

    int prefixSize = prefix.size();

    if (hadSpace) {
        ++prefixSize;
    }

    QTextCursor tc = this->textCursor();
    tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, prefixSize);
    tc.insertText(completion);
    this->setTextCursor(tc);
}

QCompleter *ResizingTextEdit::getCompleter() const
{
    return this->completer;
}

}  // namespace chatterino
