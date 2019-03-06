#pragma once

class QStringList;

namespace chatterino
{
    bool shouldRunBrowserExtensionHost(const QStringList& args);
    void runBrowserExtensionHost();

}  // namespace chatterino
