#include "Args.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include "common/QLogging.hpp"

namespace chatterino {

Args::Args(const QApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Chatterino 2 Client for Twitch Chat");
    parser.addHelpOption();

    // Used internally by app to restart after unexpected crashes
    QCommandLineOption crashRecoveryOption("crash-recovery");
    crashRecoveryOption.setFlags(QCommandLineOption::HiddenFromHelp);

    // Added to ignore the parent-window option passed during native messaging
    QCommandLineOption parentWindowOption("parent-window");
    parentWindowOption.setFlags(QCommandLineOption::HiddenFromHelp);

    parser.addOptions({
        {{"v", "version"}, "Displays version information."},
        crashRecoveryOption,
        parentWindowOption,
    });
    parser.addOption(QCommandLineOption(
        {"c", "channels"},
        "Joins only supplied channels on startup. Use letters with colons to "
        "specify platform. Only twitch channels are supported at the moment.\n"
        "If platform isn't specified, default is Twitch.",
        "t:channel1;t:channel2;..."));

    if (!parser.parse(app.arguments()))
    {
        qCWarning(chatterinoArgs)
            << "Unhandled options:" << parser.unknownOptionNames();
    }

    if (parser.isSet("help"))
    {
        qCInfo(chatterinoArgs).noquote() << parser.helpText();
        ::exit(EXIT_SUCCESS);
    }

    const QStringList args = parser.positionalArguments();
    this->shouldRunBrowserExtensionHost =
        (args.size() > 0 && (args[0].startsWith("chrome-extension://") ||
                             args[0].endsWith(".json")));

    if (parser.isSet("c"))
    {
        QJsonArray channelArray;
        QStringList channelArgList = parser.value("c").split(";");
        for (QString channelArg : channelArgList)
        {
            // Twitch is default platform
            QString platform = "t";
            QString channelName = channelArg;

            const QRegExp regExp("(.):(.*)");
            if (regExp.indexIn(channelArg) != -1)
            {
                platform = regExp.cap(1);
                channelName = regExp.cap(2);
            }

            // Twitch (default)
            if (platform == "t")
            {
                // TODO: try not to parse JSON
                QString channelObjectString =
                    "{\"splits2\": { \"data\": { \"name\": \"" + channelName +
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
