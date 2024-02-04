#pragma once

namespace pajlada::Signals {
class SignalHolder;
}  // namespace pajlada::Signals

namespace chatterino {

class PixmapButton;
class UpdateDialog;

void initUpdateButton(PixmapButton &button,
                      pajlada::Signals::SignalHolder &signalHolder);

}  // namespace chatterino
