#include "debugcount.hpp"

namespace chatterino {
namespace util {

QMap<QString, int64_t> DebugCount::counts;
std::mutex DebugCount::mut;

}  // namespace util
}  // namespace chatterino
