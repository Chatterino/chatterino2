#pragma once

#include <functional>

namespace pajlada::Signals {
class SignalHolder;
}  // namespace pajlada::Signals

namespace chatterino {

class PixmapButton;

/// Initializes the update button
///
/// The `relayout` function gets called whenever the button visibility changes
void initUpdateButton(PixmapButton &button,
                      const std::function<void()> &relayout,
                      pajlada::Signals::SignalHolder &signalHolder);

}  // namespace chatterino
