#include "DebugCount.hpp"

namespace chatterino {

UniqueAccess<QMap<QString, int64_t>> DebugCount::counts_;

}  // namespace chatterino
