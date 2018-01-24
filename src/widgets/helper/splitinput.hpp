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
    virtual void paintEvent(QPaintEvent *) override;
    virtual void resizeEvent(QResizeEvent *) override;

    virtual void mousePressEvent(QMouseEvent *event) override;

private:
    Split *const chatWidget;
    std::unique_ptr<EmotePopup> emotePopup;

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;
    QHBoxLayout hbox;
    QVBoxLayout vbox;
    QHBoxLayout editContainer;
    ResizingTextEdit textInput;
    QLabel textLengthLabel;
    RippleEffectLabel emotesLabel;
    QStringList prevMsg;
    int prevIndex = 0;
    virtual void refreshTheme() override;

private slots:
    void editTextChanged();

    friend class Split;
};

}  // namespace widgets
}  // namespace chatterino
