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
    RestoreBackupsDialog(backup::FileData fileData, QString prevError,
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
