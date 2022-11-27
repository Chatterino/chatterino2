#pragma once

#include <pajlada/signals/signalholder.hpp>
#include <QDialog>

namespace chatterino {

class LastRunCrashDialog : public QDialog
{
public:
    LastRunCrashDialog();

private:
    pajlada::Signals::SignalHolder signalHolder_;
};

}  // namespace chatterino
