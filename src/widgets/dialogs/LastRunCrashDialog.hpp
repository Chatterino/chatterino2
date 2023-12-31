#pragma once

#include <QDialog>

namespace chatterino {

class Args;

class LastRunCrashDialog : public QDialog
{
public:
    explicit LastRunCrashDialog(const Args &args);
};

}  // namespace chatterino
