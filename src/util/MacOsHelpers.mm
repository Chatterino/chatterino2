#include "util/MacOsHelpers.h"

#include <AppKit/AppKit.h>

void chatterinoSetMacOsActivationPolicyProhibited()
{
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyProhibited];
}

