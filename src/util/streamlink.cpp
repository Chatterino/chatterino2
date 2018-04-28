#include "util/streamlink.hpp"

#include "application.hpp"
#include "helpers.hpp"
#include "singletons/settingsmanager.hpp"
#include "widgets/qualitypopup.hpp"

#include <QFileInfo>
#include <QProcess>

#include <functional>

namespace chatterino {
namespace streamlink {

namespace {

const char *GetBinaryName()
{
#ifdef _WIN32
    return "streamlink.exe";
#else
    return "streamlink";
#endif
}

const char *GetDefaultBinaryPath()
{
#ifdef _WIN32
    return "C:\\Program Files (x86)\\Streamlink\\bin\\streamlink.exe";
#else
    return "/usr/bin/streamlink";
#endif
}

bool CheckStreamlinkPath(const QString &path)
{
    QFileInfo fileinfo(path);

    if (!fileinfo.exists()) {
        return false;
        // throw Exception(fS("Streamlink path ({}) is invalid, file does not exist", path));
    }

    if (fileinfo.isDir() || !fileinfo.isExecutable()) {
        return false;
    }

    return true;
}

// TODO: Make streamlink binary finder smarter
QString GetStreamlinkBinaryPath()
{
    auto app = getApp();

    QString settingPath = app->settings->streamlinkPath;

    QStringList paths;
    paths << settingPath;
    paths << GetDefaultBinaryPath();
#ifdef _WIN32
    paths << settingPath + "\\" + GetBinaryName();
    paths << settingPath + "\\bin\\" + GetBinaryName();
#else
    paths << "/usr/local/bin/streamlink";
    paths << "/bin/streamlink";
#endif

    for (const auto &path : paths) {
        if (CheckStreamlinkPath(path)) {
            return path;
        }
    }

    throw Exception("Unable to find streamlink binary. Install streamlink or set the binary path "
                    "in the settings dialog.");
}

void GetStreamQualities(const QString &channelURL, std::function<void(QStringList)> cb)
{
    QString path = GetStreamlinkBinaryPath();

    // XXX: Memory leak
    QProcess *p = new QProcess();

    QObject::connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [=](int) {
        QString lastLine = QString(p->readAllStandardOutput());
        lastLine = lastLine.trimmed().split('\n').last().trimmed();
        if (lastLine.startsWith("Available streams: ")) {
            QStringList options;
            QStringList split = lastLine.right(lastLine.length() - 19).split(", ");

            for (int i = split.length() - 1; i >= 0; i--) {
                QString option = split.at(i);
                if (option.endsWith(" (worst)")) {
                    options << option.left(option.length() - 8);
                } else if (option.endsWith(" (best)")) {
                    options << option.left(option.length() - 7);
                } else {
                    options << option;
                }
            }

            cb(options);
        }
    });

    p->start(path, {channelURL, "--default-stream=KKona"});
}

}  // namespace

void OpenStreamlink(const QString &channelURL, const QString &quality, QStringList extraArguments)
{
    auto app = getApp();

    QString path = GetStreamlinkBinaryPath();

    QStringList arguments;

    QString additionalOptions = app->settings->streamlinkOpts.getValue();
    if (!additionalOptions.isEmpty()) {
        arguments << app->settings->streamlinkOpts;
    }

    arguments.append(extraArguments);

    arguments << channelURL;

    if (!quality.isEmpty()) {
        arguments << quality;
    }

    QProcess::startDetached(path, arguments);
}

void Start(const QString &channel)
{
    auto app = getApp();

    QString channelURL = "twitch.tv/" + channel;

    QString preferredQuality = app->settings->preferredQuality;
    preferredQuality = preferredQuality.toLower();

    if (preferredQuality == "choose") {
        GetStreamQualities(channelURL, [=](QStringList qualityOptions) {
            widgets::QualityPopup::showDialog(channel, qualityOptions);
        });

        return;
    }

    QStringList args;

    // Quality converted from Chatterino format to Streamlink format
    QString quality;
    // Streamlink qualities to exclude
    QString exclude;

    if (preferredQuality == "high") {
        exclude = ">720p30";
        quality = "high,best";
    } else if (preferredQuality == "medium") {
        exclude = ">540p30";
        quality = "medium,best";
    } else if (preferredQuality == "low") {
        exclude = ">360p30";
        quality = "low,best";
    } else if (preferredQuality == "audio only") {
        quality = "audio,audio_only";
    } else {
        quality = "best";
    }
    if (!exclude.isEmpty()) {
        args << "--stream-sorting-excludes" << exclude;
    }

    OpenStreamlink(channelURL, quality, args);
}

}  // namespace streamlink
}  // namespace chatterino
