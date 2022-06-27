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

namespace chatterino {

class Split;
class EmotePopup;
class InputCompletionPopup;
class EffectLabel;
class ResizingTextEdit;

class SplitInput : public BaseWidget
{
    Q_OBJECT

public:
    SplitInput(Split *_chatWidget);

    void clearSelection();
    bool isEditFirstWord() const;
    QString getInputText() const;
    void insertText(const QString &text);

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

private:
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
    } ui_;

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
};

}  // namespace chatterino
