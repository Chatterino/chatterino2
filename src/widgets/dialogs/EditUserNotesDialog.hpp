// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
    static int currentWordPosition(const QTextCursor &cursor);
    static bool isBold(const QString &line, const int pos);
    static bool isItalic(const QString &line, const int pos);
    static bool isHeading(const QString &line, const int pos);

    QTextEdit *textEdit_{};
    QCheckBox *previewCheckBox_{};
    QSplitter *splitter_{};
    MarkdownLabel *previewLabel_{};
};

}  // namespace chatterino
