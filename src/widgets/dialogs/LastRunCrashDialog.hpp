#pragma once

#include <QDialog>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class LastRunCrashDialog : public QDialog
{
public:
    LastRunCrashDialog();

private:
    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
