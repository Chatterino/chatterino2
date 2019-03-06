#pragma once

#include <QCompleter>
#include <QKeyEvent>
#include <QTextEdit>

namespace chatterino::ui
{
    class ResizingTextEdit : public QTextEdit
    {
        Q_OBJECT

    public:
        ResizingTextEdit();

        QSize sizeHint() const override;

        bool hasHeightForWidth() const override;

        // pajlada::Signals::Signal<QKeyEvent*> keyPressed;
        // pajlada::Signals::NoArgSignal focused;
        // pajlada::Signals::NoArgSignal focusLost;

        void setCompleter(QCompleter* c);
        QCompleter* getCompleter() const;

    signals:
        void focused();
        void unfocused();

    protected:
        int heightForWidth(int) const override;
        void keyPressEvent(QKeyEvent* event) override;

        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

    private:
        // hadSpace is set to true in case the "textUnderCursor" word was after
        // a space
        QString textUnderCursor(bool* hadSpace = nullptr) const;

        // QCompleter* completer_ = nullptr;
        // bool completionInProgress_ = false;

    private slots:
        void insertCompletion(const QString& completion);
    };

}  // namespace chatterino::ui
