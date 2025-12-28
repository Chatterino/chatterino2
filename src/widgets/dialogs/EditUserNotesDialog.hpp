#pragma once

#include "pajlada/signals/signal.hpp"
#include "widgets/BasePopup.hpp"

class QCheckBox;
class QSplitter;
class QTextCursor;
class QTextEdit;

namespace chatterino {

class MarkdownLabel;

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
    int currentWordPosition(const QTextCursor &cursor);
    bool isBold(const QString &line, const int pos);
    bool isItalic(const QString &line, const int pos);
    bool isHeading(const QString &line, const int pos);

    QTextEdit *textEdit_{};
    QCheckBox *previewCheckBox_{};
    QSplitter *splitter_{};
    MarkdownLabel *previewLabel_{};
};

}  // namespace chatterino
