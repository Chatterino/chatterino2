#pragma once

#include "pajlada/signals/signal.hpp"
#include "widgets/BasePopup.hpp"

class QCheckBox;
class QSplitter;
class QTextEdit;

namespace chatterino {

class Label;

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
    void updatePreview();

    QTextEdit *textEdit_{};
    QCheckBox *previewCheckBox_{};
    QSplitter *splitter_{};
    Label *previewLabel_{};
};

}  // namespace chatterino
