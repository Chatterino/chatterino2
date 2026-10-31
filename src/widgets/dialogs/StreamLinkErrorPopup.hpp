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
    static void showError(const QString &url, const QString &errorMessage);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    StreamLinkErrorPopup(const QString &url, const QString &errorMessage);

    void okButtonClicked();

    struct {
        QVBoxLayout *vbox;
        QLabel *label;
        QTextEdit *textEdit;
        QDialogButtonBox *buttonBox;
    } ui_{};
};

}  // namespace chatterino
