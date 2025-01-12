#pragma once

#include <QString>

namespace chatterino::incognitobrowser::detail {

QString getPrivateSwitch(const QString &browserExecutable);

}  // namespace chatterino::incognitobrowser::detail

namespace chatterino {

bool supportsIncognitoLinks();
bool openLinkIncognito(const QString &link);

}  // namespace chatterino
