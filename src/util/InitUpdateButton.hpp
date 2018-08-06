#pragma once

#include <memory>

namespace pajlada {
namespace Signals {
class SignalHolder;
}
}  // namespace pajlada

namespace chatterino {

class RippleEffectButton;
class UpdateDialog;

void initUpdateButton(RippleEffectButton &button,
                      std::unique_ptr<UpdateDialog> &handle,
                      pajlada::Signals::SignalHolder &signalHolder);

}  // namespace chatterino
