#pragma once

#include "util/QObjectRef.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/dialogs/EmotePopup.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

namespace chatterino {

class Split;
class EmotePopup;
class InputCompletionPopup;
class EffectLabel;
class MessageThread;
class ResizingTextEdit;

class SplitInput : public BaseWidget
{
    Q_OBJECT

public:
    SplitInput(Split *_chatWidget);
    SplitInput(QWidget *parent, Split *_chatWidget);

    void clearSelection();
    bool isEditFirstWord() const;
    QString getInputText() const;
    void insertText(const QString &text);

    void setReply(const std::shared_ptr<MessageThread> &reply);

    /**
     * @brief Hide the widget
     *
     * This is a no-op if the SplitInput is already hidden
     **/
    void hide();

    /**
     * @brief Show the widget
     *
     * This is a no-op if the SplitInput is already shown
     **/
    void show();

    /**
     * @brief Returns the hidden or shown state of the SplitInput
     *
     * Hidden in this context means "has 0 height", meaning it won't be visible
     * but Qt still treats the widget as "technically visible" so we receive events
     * as if the widget is visible
     **/
    bool isHidden() const;

    pajlada::Signals::Signal<const QString &> textChanged;

protected:
    void scaleChangedEvent(float scale_) override;
    void themeChangedEvent() override;

    void paintEvent(QPaintEvent * /*event*/) override;
    void resizeEvent(QResizeEvent * /*event*/) override;

    virtual void giveFocus(Qt::FocusReason reason);

    void postMessageSend(const QString &message,
                         const std::vector<QString> &arguments);

protected:
    virtual QString hotkeyCursorToStart(std::vector<QString> &arguments);
    virtual QString hotkeyCursorToEnd(std::vector<QString> &arguments);
    virtual QString hotkeyOpenEmotesPopup();
    virtual QString hotkeySendMessage(std::vector<QString> &arguments);
    virtual QString hotkeyPreviousMessage();
    virtual QString hotkeyNextMessage();
    virtual QString hotkeyUndo();
    virtual QString hotkeyRedo();
    virtual QString hotkeyCopy(std::vector<QString> &arguments);
    virtual QString hotkeyPaste();
    virtual QString hotkeyClear();
    virtual QString hotkeySelectAll();
    virtual QString hotkeySelectWord();

    void addShortcuts() override;
    void initLayout();
    bool eventFilter(QObject *obj, QEvent *event) override;
    void installKeyPressedEvent();
    void onCursorPositionChanged();
    void onTextChanged();
    void updateEmoteButton();
    void updateCompletionPopup();
    void showCompletionPopup(const QString &text, bool emoteCompletion);
    void hideCompletionPopup();
    void insertCompletionText(const QString &text);
    void openEmotePopup();

    void updateCancelReplyButton();
    int replyBottomPadding() const;

    // scaledMaxHeight returns the height in pixels that this widget can grow to
    // This does not take hidden into account, so callers must take hidden into account themselves
    int scaledMaxHeight() const;

    Split *const split_;
    QObjectRef<EmotePopup> emotePopup_;
    QObjectRef<InputCompletionPopup> inputCompletionPopup_;

    struct {
        ResizingTextEdit *textEdit;
        QLabel *textEditLength;
        EffectLabel *emoteButton;

        QHBoxLayout *hbox;
        QVBoxLayout *vbox;

        QHBoxLayout *replyHbox;
        QLabel *replyLabel;
        EffectLabel *cancelReplyButton;
    } ui_;

    std::shared_ptr<MessageThread> replyThread_ = nullptr;

    pajlada::Signals::SignalHolder managedConnections_;
    QStringList prevMsg_;
    QString currMsg_;
    int prevIndex_ = 0;

    // Hidden denotes whether this split input should be hidden or not
    // This is used instead of the regular QWidget::hide/show because
    // focus events don't work as expected, so instead we use this bool and
    // set the height of the split input to 0 if we're supposed to be hidden instead
    bool hidden{false};

private slots:
    void editTextChanged();

    friend class Split;
    friend class ReplyThreadPopup;
};

}  // namespace chatterino
