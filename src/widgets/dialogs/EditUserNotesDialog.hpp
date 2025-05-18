#pragma once

#include "pajlada/signals/signal.hpp"
#include "widgets/BasePopup.hpp"

class QTextEdit;

namespace chatterino {

class EditUserNotesDialog : public BasePopup
{
    Q_OBJECT

public:
    EditUserNotesDialog(QWidget *parent = nullptr);

    void setNotes(const QString &initialNotes);
    void updateWindowTitle(const QString &displayUsername);

    pajlada::Signals::Signal<const QString &> onOk;

protected:
    void showEvent(QShowEvent *event) override;
    void themeChangedEvent() override;

private:
    QTextEdit *textEdit_{};
};

}  // namespace chatterino
