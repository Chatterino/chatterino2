#pragma once

#include <QDialog>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {
namespace widgets {

class LastRunCrashDialog : public QDialog, pajlada::Signals::SignalHolder
{
public:
    LastRunCrashDialog();
};

}  // namespace widgets
}  // namespace chatterino
