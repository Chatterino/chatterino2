#pragma once

#include <QDialog>

#include <filesystem>
#include <map>

namespace chatterino {

class Paths;

class RestoreBackupsDialog : public QDialog
{
public:
    RestoreBackupsDialog(const Paths *paths, QWidget *parent = nullptr);

private:
    struct FilePlan {
        std::filesystem::path backupPath;
        std::filesystem::path restorePath;
    };
    std::map<QString, FilePlan> plans;
};

}  // namespace chatterino
