#include "DebugCount.hpp"

namespace chatterino {

QMap<QString, int64_t> DebugCount::counts_;
std::mutex DebugCount::mut_;

}  // namespace chatterino
