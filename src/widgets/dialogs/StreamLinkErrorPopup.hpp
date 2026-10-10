// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BasePopup.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

namespace chatterino {

class StreamLinkErrorPopup : public BasePopup
{
public:
    StreamLinkErrorPopup(const QString &url, const QString &standardOutput,
                         const QString &standardError);
    static void showError(const QString &url, const QString &standardOutput,
                          const QString &standardError);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void okButtonClicked();

    struct {
        QVBoxLayout *vbox;
        QLabel *infoLabel;
        QLabel *stdoutLabel;
        QTextEdit *stdoutTextEdit;
        QLabel *stderrLabel;
        QTextEdit *stderrTextEdit;
        QDialogButtonBox *buttonBox;
    } ui_{};
};

}  // namespace chatterino
