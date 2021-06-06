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
class EmoteInputPopup;
class UsernameInputPopup;
class EffectLabel;
class ResizingTextEdit;

class SplitInput : public BaseWidget
{
    Q_OBJECT

public:
    SplitInput(Split *_chatWidget);

    void clearSelection();
    QString getInputText() const;
    void insertText(const QString &text);

    pajlada::Signals::Signal<const QString &> textChanged;

protected:
    virtual void scaleChangedEvent(float scale_) override;
    virtual void themeChangedEvent() override;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    void initLayout();
    void installKeyPressedEvent();
    void onCursorPositionChanged();
    void onTextChanged();
    void updateEmoteButton();
    void showColonMenu(const QString &text);
    void hideColonMenu();
    void updateCompletionMenus();
    void showUsernameMenu(const QString &text);
    void hideUsernameMenu();
    void insertCompletionText(const QString &text);
    void openEmotePopup();

    Split *const split_;
    QObjectRef<EmotePopup> emotePopup_;
    QObjectRef<EmoteInputPopup> emoteInputPopup_;
    QObjectRef<UsernameInputPopup> usernameInputPopup_;

    struct {
        ResizingTextEdit *textEdit;
        QLabel *textEditLength;
        EffectLabel *emoteButton;

        QHBoxLayout *hbox;
    } ui_;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
    QStringList prevMsg_;
    QString currMsg_;
    int prevIndex_ = 0;

private slots:
    void editTextChanged();

    friend class Split;
};

}  // namespace chatterino
