#pragma once

#include <QDialog>

namespace chatterino {

class Args;
class Paths;

class LastRunCrashDialog : public QDialog
{
public:
    explicit LastRunCrashDialog(const Args &args, const Paths &paths);
};

}  // namespace chatterino
