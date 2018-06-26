#include "DebugCount.hpp"

namespace chatterino {

QMap<QString, int64_t> DebugCount::counts;
std::mutex DebugCount::mut;

}  // namespace chatterino
