// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "util/Backup.hpp"
#include "util/Expected.hpp"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

namespace chatterino {

class Paths;

class RestoreBackupsDialog : public QDialog
{
public:
    RestoreBackupsDialog(backup::FileData fileData, const QString &prevError,
                         QWidget *parent = nullptr);

private:
    void refreshBackups();

    backup::FileData *selectedFileData() const;

    backup::FileData fileData;

    QComboBox backupCombo;
    QPushButton showButton;
    QDialogButtonBox dialogButtons;
    QLabel corruptedBackupsWarning;
};

}  // namespace chatterino
