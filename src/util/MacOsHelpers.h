#pragma once

#include <qglobal.h>

void chatterinoSetMacOsActivationPolicyProhibited();

namespace chatterino {

#ifdef Q_OS_DARWIN
QString getMacOSDefaultBrowserPath();
#endif

}  // namespace chatterino