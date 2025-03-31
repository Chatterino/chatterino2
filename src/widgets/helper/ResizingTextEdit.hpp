#pragma once

#include <pajlada/signals/signal.hpp>
#include <QCompleter>
#include <QKeyEvent>
#include <QTextEdit>

namespace chatterino {

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit();

    QSize sizeHint() const override;

    bool hasHeightForWidth() const override;
    bool isFirstWord() const;

    pajlada::Signals::Signal<QKeyEvent *> keyPressed;
    pajlada::Signals::NoArgSignal focused;
    pajlada::Signals::NoArgSignal focusLost;
    pajlada::Signals::Signal<const QMimeData *> imagePasted;

    void setCompleter(QCompleter *c);
    /**
     * Resets a completion for this text if one was is progress.
     * See `completionInProgress_`.
     */
    void resetCompletion();

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
    /**
     * This is true if a completion was done but the user didn't type yet,
     * and might want to press `Tab` again to get the next completion
     * on the original text.
     *
     * For example:
     *
     * input: "pog"
     * `Tab` pressed:
     *   - complete to "PogBones"
     *   - retain "pog" for next completion
     *   - set `completionInProgress_ = true`
     * `Tab` pressed again:
     *   - complete ["pog"] to "PogChamp"
     *
     * [other key] pressed or cursor moved - updating the input text:
     *   - set `completionInProgress_ = false`
     */
    bool completionInProgress_ = false;

    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void insertCompletion(const QString &completion);
};

}  // namespace chatterino
