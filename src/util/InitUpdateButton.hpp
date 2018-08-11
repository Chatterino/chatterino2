#pragma once

#include <memory>

namespace pajlada {
namespace Signals {
class SignalHolder;
}
}  // namespace pajlada

namespace chatterino {

class Button;
class UpdateDialog;

void initUpdateButton(Button &button,
                      std::unique_ptr<UpdateDialog> &handle,
                      pajlada::Signals::SignalHolder &signalHolder);

}  // namespace chatterino
