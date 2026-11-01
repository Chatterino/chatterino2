#pragma once

#include <QString>

class QWidget;

void chatterinoSetMacOsActivationPolicyProhibited();

namespace chatterino {

#ifdef Q_OS_DARWIN
QString getMacOSDefaultBrowserPath();
void setMacOSWindowManaged(QWidget &widget);
#endif

}  // namespace chatterino
