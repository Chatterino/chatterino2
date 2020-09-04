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
    //parser.addVersionOption();
    QCommandLineOption versionOption(QStringList() << "v"
                                                   << "version",
                                     "Displays version information.");
    QCommandLineOption crashRecoveryOption(
        "crash-recovery",
        "Used internally by app itself to restart after unexpected crashes.");
    QCommandLineOption channelsOption(
        QStringList() << "c"
                      << "channels",
        "Tells which channels to join on startup.", "channel1,channel2,...");

    parser.addOption(versionOption);
    parser.addOption(crashRecoveryOption);
    parser.addOption(channelsOption);
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    this->shouldRunBrowserExtensionHost =
        (args.size() > 0 && (args[0].startsWith("chrome-extension://") ||
                             args[0].endsWith(".json")));

    if (parser.isSet(channelsOption))
    {
        QStringList channelList = parser.value(channelsOption).split(",");
        QJsonArray channelArray;
        for (QString channel : channelList)
        {
            QString channelObjectString =
                "{\"splits2\": { \"data\": { \"name\": \"" + channel +
                "\", \"type\": "
                "\"twitch\" }, \"type\": \"split\" }}";
            QJsonObject channelObject;
            QJsonDocument doc =
                QJsonDocument::fromJson(channelObjectString.toUtf8());
            channelObject = doc.object();
            channelArray.push_back(channelObject);
        };
        this->joinArgumentChannels = true;
        this->channelsToJoin = channelArray;
    }

    this->printVersion = parser.isSet(versionOption);
    this->crashRecovery = parser.isSet(crashRecoveryOption);
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
