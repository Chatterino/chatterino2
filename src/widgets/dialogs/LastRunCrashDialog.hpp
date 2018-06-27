#pragma once

#include <QDialog>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class LastRunCrashDialog : public QDialog, pajlada::Signals::SignalHolder
{
public:
    LastRunCrashDialog();
};

}  // namespace chatterino
