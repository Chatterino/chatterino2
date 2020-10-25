#include "widgets/helper/ResizingTextEdit.hpp"

#include "common/Common.hpp"
#include "common/CompletionModel.hpp"
#include "singletons/Settings.hpp"

#include <QMimeData>

namespace chatterino {

ResizingTextEdit::ResizingTextEdit()
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
    this->setSizePolicy(sizePolicy);
    this->setAcceptRichText(false);

    QObject::connect(this, &QTextEdit::textChanged, this,
                     &QWidget::updateGeometry);

    // Whenever the setting for emote completion changes, force a
    // refresh on the completion model the next time "Tab" is pressed
    getSettings()->prefixOnlyEmoteCompletion.connect(
        [this] { this->completionInProgress_ = false; });

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
    if (words.size() == 0)
    {
        return QString();
    }

    bool first = true;
    QString lastWord;
    for (auto it = words.crbegin(); it != words.crend(); ++it)
    {
        auto word = *it;

        if (first && word.isEmpty())
        {
            first = false;
            if (hadSpace != nullptr)
            {
                *hadSpace = true;
            }
            continue;
        }

        lastWord = word.toString();
        break;
    }

    if (lastWord.isEmpty())
    {
        return QString();
    }

    return lastWord;
}

void ResizingTextEdit::keyPressEvent(QKeyEvent *event)
{
    event->ignore();

    this->keyPressed.invoke(event);

    bool doComplete =
        (event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backtab) &&
        (event->modifiers() & Qt::ControlModifier) == Qt::NoModifier &&
        !event->isAccepted();

    if (doComplete)
    {
        // check if there is a completer
        if (!this->completer_)
        {
            return;
        }

        QString currentCompletionPrefix = this->textUnderCursor();
        bool isFirstWord = [&] {
            QString plainText = this->toPlainText();
            for (int i = this->textCursor().position(); i >= 0; i--)
            {
                if (plainText[i] == ' ')
                {
                    return false;
                }
            }
            return true;
        }();

        // check if there is something to complete
        if (currentCompletionPrefix.size() <= 1)
        {
            return;
        }

        auto *completionModel =
            static_cast<CompletionModel *>(this->completer_->model());

        if (!this->completionInProgress_)
        {
            // First type pressing tab after modifying a message, we refresh our
            // completion model
            this->completer_->setModel(completionModel);
            completionModel->refresh(currentCompletionPrefix, isFirstWord);
            this->completionInProgress_ = true;
            this->completer_->setCompletionPrefix(currentCompletionPrefix);
            this->completer_->complete();
            return;
        }

        // scrolling through selections
        if (event->key() == Qt::Key_Tab)
        {
            if (!this->completer_->setCurrentRow(
                    this->completer_->currentRow() + 1))
            {
                // wrap over and start again
                this->completer_->setCurrentRow(0);
            }
        }
        else
        {
            if (!this->completer_->setCurrentRow(
                    this->completer_->currentRow() - 1))
            {
                // wrap over and start again
                this->completer_->setCurrentRow(
                    this->completer_->completionCount() - 1);
            }
        }

        this->completer_->complete();
        return;
    }

    if (!event->text().isEmpty())
    {
        this->completionInProgress_ = false;
    }

    if (!event->isAccepted())
    {
        QTextEdit::keyPressEvent(event);
    }
}

void ResizingTextEdit::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);

    if (event->gotFocus())
    {
        this->focused.invoke();
    }
}

void ResizingTextEdit::focusOutEvent(QFocusEvent *event)
{
    QTextEdit::focusOutEvent(event);

    if (event->lostFocus())
    {
        this->focusLost.invoke();
    }
}

void ResizingTextEdit::setCompleter(QCompleter *c)
{
    if (this->completer_)
    {
        QObject::disconnect(this->completer_, nullptr, this, nullptr);
    }

    this->completer_ = c;

    if (!this->completer_)
    {
        return;
    }

    this->completer_->setWidget(this);
    this->completer_->setCompletionMode(QCompleter::InlineCompletion);
    this->completer_->setCaseSensitivity(Qt::CaseInsensitive);

    if (getSettings()->prefixOnlyEmoteCompletion)
    {
        this->completer_->setFilterMode(Qt::MatchStartsWith);
    }
    else
    {
        this->completer_->setFilterMode(Qt::MatchContains);
    }

    QObject::connect(completer_,
                     static_cast<void (QCompleter::*)(const QString &)>(
                         &QCompleter::highlighted),
                     this, &ResizingTextEdit::insertCompletion);
}

void ResizingTextEdit::insertCompletion(const QString &completion)
{
    if (this->completer_->widget() != this)
    {
        return;
    }

    bool hadSpace = false;
    auto prefix = this->textUnderCursor(&hadSpace);

    int prefixSize = prefix.size();

    if (hadSpace)
    {
        ++prefixSize;
    }

    QTextCursor tc = this->textCursor();
    tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor,
                    prefixSize);
    tc.insertText(completion);
    this->setTextCursor(tc);
}

bool ResizingTextEdit::canInsertFromMimeData(const QMimeData *source) const
{
    if (source->hasImage() || source->hasFormat("text/plain"))
    {
        return true;
    }
    return QTextEdit::canInsertFromMimeData(source);
}

void ResizingTextEdit::insertFromMimeData(const QMimeData *source)
{
    if (source->hasImage() || source->hasUrls())
    {
        this->imagePasted.invoke(source);
    }
    else
    {
        insertPlainText(source->text());
    }
}

QCompleter *ResizingTextEdit::getCompleter() const
{
    return this->completer_;
}

}  // namespace chatterino
