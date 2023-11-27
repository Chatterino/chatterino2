#pragma once

#include "messages/Message.hpp"
#include "widgets/BaseWidget.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QPointer>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

#include <memory>

namespace chatterino {

class Split;
class EmotePopup;
class InputCompletionPopup;
class EffectLabel;
class ResizingTextEdit;
class ChannelView;
enum class CompletionKind;

class SplitInput : public BaseWidget
{
    Q_OBJECT

public:
    SplitInput(Split *_chatWidget, bool enableInlineReplying = true);
    SplitInput(QWidget *parent, Split *_chatWidget, ChannelView *_channelView,
               bool enableInlineReplying = true);

    bool hasSelection() const;
    void clearSelection() const;

    bool isEditFirstWord() const;
    QString getInputText() const;
    void insertText(const QString &text);

    void setReply(MessagePtr reply, bool showInlineReplying = true);
    void setPlaceholderText(const QString &text);

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
    pajlada::Signals::NoArgSignal selectionChanged;

protected:
    void scaleChangedEvent(float scale_) override;
    void themeChangedEvent() override;

    void paintEvent(QPaintEvent * /*event*/) override;
    void resizeEvent(QResizeEvent * /*event*/) override;

    void mousePressEvent(QMouseEvent *event) override;

    virtual void giveFocus(Qt::FocusReason reason);

    QString handleSendMessage(std::vector<QString> &arguments);
    void postMessageSend(const QString &message,
                         const std::vector<QString> &arguments);

    /// Clears the input box, clears reply thread if inline replies are enabled
    void clearInput();

protected:
    void addShortcuts() override;
    void initLayout();
    bool eventFilter(QObject *obj, QEvent *event) override;
#ifdef DEBUG
    bool keyPressedEventInstalled{};
#endif
    void installKeyPressedEvent();
    void onCursorPositionChanged();
    void onTextChanged();
    void updateEmoteButton();
    void updateCompletionPopup();
    void showCompletionPopup(const QString &text, CompletionKind kind);
    void hideCompletionPopup();
    void insertCompletionText(const QString &input_) const;
    void openEmotePopup();

    void updateCancelReplyButton();

    // scaledMaxHeight returns the height in pixels that this widget can grow to
    // This does not take hidden into account, so callers must take hidden into account themselves
    int scaledMaxHeight() const;

    // Returns true if the channel this input is connected to is a Twitch channel,
    // the user's setting is set to Prevent, and the given text goes beyond the Twitch message length limit
    bool shouldPreventInput(const QString &text) const;

    Split *const split_;
    ChannelView *const channelView_;
    QPointer<EmotePopup> emotePopup_;
    QPointer<InputCompletionPopup> inputCompletionPopup_;

    struct {
        ResizingTextEdit *textEdit;
        QLabel *textEditLength;
        EffectLabel *sendButton;
        EffectLabel *emoteButton;

        QHBoxLayout *hbox;
        QVBoxLayout *vbox;

        QWidget *replyWrapper;
        QHBoxLayout *replyHbox;
        QLabel *replyLabel;
        EffectLabel *cancelReplyButton;
    } ui_{};

    MessagePtr replyThread_ = nullptr;
    bool enableInlineReplying_;

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
