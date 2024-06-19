#include "singletons/Resources.hpp"

#include "debug/AssertInGuiThread.hpp"

namespace {

using namespace chatterino;

static Resources2 *resources = nullptr;

}  // namespace

namespace chatterino {

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
