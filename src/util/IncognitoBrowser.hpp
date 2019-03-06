#pragma once

#include <QtGlobal>

namespace chatterino
{
    bool supportsIncognitoLinks();
    void openLinkIncognito(const QString& link);

}  // namespace chatterino
