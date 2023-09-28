#pragma once

#include <pajlada/signals/signalholder.hpp>
#include <QDialog>

namespace chatterino {

class LastRunCrashDialog : public QDialog
{
public:
    LastRunCrashDialog();
};

}  // namespace chatterino
