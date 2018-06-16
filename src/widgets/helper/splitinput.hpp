#pragma once

#include "resizingtextedit.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/emotepopup.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPaintEvent>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {
namespace widgets {

class Split;

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
    virtual void scaleChangedEvent(float scale) override;
    virtual void themeRefreshEvent() override;

    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    Split *const split_;
    std::unique_ptr<EmotePopup> emotePopup_;

    struct {
        ResizingTextEdit *textEdit;
        QLabel *textEditLength;
        RippleEffectLabel *emoteButton;

        QHBoxLayout *hbox;
    } ui_;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
    QStringList prevMsg_;
    QString currMsg_;
    int prevIndex_ = 0;

    void initLayout();
    void installKeyPressedEvent();

    void updateEmoteButton();

private slots:
    void editTextChanged();

    friend class Split;
};

}  // namespace widgets
}  // namespace chatterino
