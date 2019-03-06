#include "util/Resources.hpp"

namespace chatterino
{
    Resources2& resources()
    {
        static Resources2 item;
        return item;
    }
}  // namespace chatterino
