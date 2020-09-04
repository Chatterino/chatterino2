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
    parser.addOptions({
        {{"v", "version"}, "Displays version information."},
        {"crash-recovery",
         "Used internally by app to restart after unexpected crashes."},
    });
    parser.addOption(QCommandLineOption(
        {"c", "channels"}, "Join only supplied channels on startup.",
        "channel1,channel2,..."));
    parser.process(app);

    const QStringList args = parser.positionalArguments();
    this->shouldRunBrowserExtensionHost =
        (args.size() > 0 && (args[0].startsWith("chrome-extension://") ||
                             args[0].endsWith(".json")));

    if (parser.isSet("c"))
    {
        QStringList channelList = parser.value("c").split(",");
        QJsonArray channelArray;
        for (QString channel : channelList)
        {
            // TODO: maybe this can be improved to not use strings?
            QString channelObjectString =
                "{\"splits2\": { \"data\": { \"name\": \"" + channel +
                "\", \"type\": \"twitch\" }, \"type\": \"split\" }}";
            channelArray.push_back(
                QJsonDocument::fromJson(channelObjectString.toUtf8()).object());
        };
        this->joinArgumentChannels = true;
        qDebug() << channelArray;
        this->channelsToJoin = channelArray;
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
