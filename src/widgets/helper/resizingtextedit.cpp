#include "widgets/helper/resizingtextedit.hpp"
#include "completionmanager.hpp"

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

    this->keyPressed(event);

    if (event->key() == Qt::Key_Backtab) {
        // Ignore for now. We want to use it for autocomplete later
        return;
    }

    if (event->key() == Qt::Key_Tab &&
        (event->modifiers() & Qt::ControlModifier) == Qt::NoModifier) {
        QString currentCompletionPrefix = this->textUnderCursor();

        if (!currentCompletionPrefix.size()) {
            return;
        }

        auto *completionModel =
            static_cast<chatterino::CompletionModel *>(this->completer->model());
        completionModel->refresh();

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
