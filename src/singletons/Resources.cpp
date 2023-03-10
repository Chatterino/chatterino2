#include "singletons/Resources.hpp"

#include "debug/AssertInGuiThread.hpp"

namespace chatterino {
namespace {
    static Resources2 *resources = nullptr;
}

Resources2 &getResources()
{
    assert(resources);

    return *resources;
}

void initResources()
{
    assertInGuiThread();

    resources = new Resources2;
}

}  // namespace chatterino
