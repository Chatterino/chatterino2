#pragma once

#include <QString>

namespace chatterino {

bool supportsIncognitoLinks();
bool openLinkIncognito(const QString &link);

}  // namespace chatterino
