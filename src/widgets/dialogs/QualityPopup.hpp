// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BasePopup.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace chatterino {

class QualityPopup : public BasePopup
{
public:
    QualityPopup(const QString &url, QStringList options);
    static void showDialog(const QString &url, QStringList options);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void okButtonClicked();
    void cancelButtonClicked();

    struct {
        QVBoxLayout *vbox;
        QComboBox *selector;
        QDialogButtonBox *buttonBox;
    } ui_{};

    QString url_;
};

}  // namespace chatterino
