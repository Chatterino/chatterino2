#include "incognitobrowser.hpp"

#include <QProcess>
#include <QSettings>
#include <QVariant>

#include "debug/Log.hpp"

#ifdef Q_OS_WIN

namespace chatterino {
namespace {
    QString getAppPath(const QString &executableName)
    {
        auto paths = QStringList{
            // clang-format off
            "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" + executableName,
            "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" + executableName
            // clang-format on
        };

        for (const auto &path : paths) {
            auto val = QSettings(path, QSettings::NativeFormat)
                           .value("Default")
                           .toString();
            if (!val.isNull()) {
                return val;
            }
        }

        return QString();
    }
}  // namespace

void openLinkIncognito(const QString &link)
{
    auto browserChoice = QSettings("HKEY_CURRENT_"
                                   "USER\\Software\\Microsoft\\Windows\\Shell\\"
                                   "Associations\\UrlAssociatio"
                                   "ns\\http\\UserChoice",
                                   QSettings::NativeFormat)
                             .value("Progid")
                             .toString();

    if (!browserChoice.isNull()) {
        if (browserChoice == "FirefoxURL") {
            // Firefox
            auto path = getAppPath("firefox.exe");

            QProcess::startDetached(path, {"-private-window", link});
        } else if (browserChoice == "ChromeHTML") {
            // Chrome
            auto path = getAppPath("chrome.exe");

            QProcess::startDetached(path, {"-incognito", link});
        }
        // Possible implementation for MS Edge.
        // Doesn't quite work yet.
        /*
            else if (browserChoice == "AppXq0fevzme2pys62n3e0fbqa7peapykr8v") {
                // Edge
                 QProcess::startDetached("C:\\Windows\\System32\\cmd.exe",
                                                    {"/c", "start",
                 "shell:AppsFolder\Microsoft.MicrosoftEdge_"
                 "8wekyb3d8bbwe!MicrosoftEdge",
                                                     "-private", link});
            }
            */
    }
}

}  // namespace chatterino

#endif
