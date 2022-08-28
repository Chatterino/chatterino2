#pragma once

#include <QtGlobal>

namespace chatterino {

bool supportsIncognitoLinks();
bool openLinkIncognito(const QString &link);

}  // namespace chatterino
