#pragma once

#include <memory>

namespace pajlada {
namespace Signals {
class SignalHolder;
}
}  // namespace pajlada

namespace chatterino {

class RippleEffectButton;
class UpdatePromptDialog;

void initUpdateButton(RippleEffectButton &button, std::unique_ptr<UpdatePromptDialog> &handle,
                      pajlada::Signals::SignalHolder &signalHolder);

}  // namespace chatterino
