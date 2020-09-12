#include "Args.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace chatterino {

Args::Args(const QApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Chatterino 2 Client for Twitch Chat");
    parser.addHelpOption();

    // Used internally by app to restart after unexpected crashes
    QCommandLineOption crashRecoveryOption("crash-recovery");
    crashRecoveryOption.setHidden(true);

    parser.addOptions({
        {{"v", "version"}, "Displays version information."},
        crashRecoveryOption,
    });
    parser.addOption(QCommandLineOption(
        {"c", "channels"},
        "Join only supplied channels on startup. Use letters with colons to "
        "specify platform. Only twitch channels are supported at the moment.",
        "t:channel1;t:channel2;..."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    this->shouldRunBrowserExtensionHost =
        (args.size() > 0 && (args[0].startsWith("chrome-extension://") ||
                             args[0].endsWith(".json")));

    if (parser.isSet("c"))
    {
        //
        QStringList channelList = parser.value("c").split(";");
        QJsonArray channelArray;
        for (QString channel : channelList)
        {
            const QRegExp regExp("(.):(.*)");
            if (regExp.indexIn(channel) == -1)
            {
                qDebug()
                    << "[CommandLineArguments] Invalid channel specification"
                    << channel;
                continue;
            }
            qDebug() << regExp.capturedTexts();

            // Twitch
            if (regExp.cap(1) == "t")
            {
                // TODO: try not to parse JSON?
                QString channelObjectString =
                    "{\"splits2\": { \"data\": { \"name\": \"" + regExp.cap(2) +
                    "\", \"type\": \"twitch\" }, \"type\": \"split\" }}";
                channelArray.push_back(
                    QJsonDocument::fromJson(channelObjectString.toUtf8())
                        .object());
            }
        }
        if (channelArray.size() > 0)
        {
            this->dontSaveSettings = true;
            this->channelsToJoin = channelArray;
        }
    }

    this->printVersion = parser.isSet("v");
    this->crashRecovery = parser.isSet("crash-recovery");
}

static Args *instance = nullptr;

void initArgs(const QApplication &app)
{
    instance = new Args(app);
}

const Args &getArgs()
{
    assert(instance);

    return *instance;
}

}  // namespace chatterino
