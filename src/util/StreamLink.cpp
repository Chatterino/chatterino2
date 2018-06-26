#include "util/StreamLink.hpp"

#include "Application.hpp"
#include "Helpers.hpp"
#include "singletons/SettingsManager.hpp"
#include "widgets/dialogs/QualityPopup.hpp"

#include <QErrorMessage>
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

QString getStreamlinkProgram()
{
    auto app = getApp();

    if (app->settings->streamlinkUseCustomPath) {
        return app->settings->streamlinkPath + "/" + GetBinaryName();
    } else {
        return GetBinaryName();
    }
}

bool CheckStreamlinkPath(const QString &path)
{
    QFileInfo fileinfo(path);

    if (!fileinfo.exists()) {
        return false;
        // throw Exception(fS("Streamlink path ({}) is invalid, file does not exist", path));
    }

    return fileinfo.isExecutable();
}

void showStreamlinkNotFoundError()
{
    static QErrorMessage *msg = new QErrorMessage;

    auto app = getApp();
    if (app->settings->streamlinkUseCustomPath) {
        msg->showMessage(
            "Unable to find Streamlink executable\nMake sure your custom path is pointing "
            "to the DIRECTORY where the streamlink executable is located");
    } else {
        msg->showMessage("Unable to find Streamlink executable.\nIf you have Streamlink "
                         "installed, you might need to enable the custom path option");
    }
}

QProcess *createStreamlinkProcess()
{
    auto p = new QProcess;
    p->setProgram(getStreamlinkProgram());

    QObject::connect(p, &QProcess::errorOccurred, [=](auto err) {
        if (err == QProcess::FailedToStart) {
            showStreamlinkNotFoundError();
        } else {
            qDebug() << "Error occured: " << err;  //
        }

        p->deleteLater();
    });

    QObject::connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [=](int res) {
        p->deleteLater();  //
    });

    return p;
}

}  // namespace

void GetStreamQualities(const QString &channelURL, std::function<void(QStringList)> cb)
{
    auto p = createStreamlinkProcess();

    QObject::connect(p, static_cast<void (QProcess::*)(int)>(&QProcess::finished), [=](int res) {
        if (res != 0) {
            qDebug() << "Got error code" << res;
            // return;
        }
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

    p->setArguments({channelURL, "--default-stream=KKona"});

    p->start();
}

void OpenStreamlink(const QString &channelURL, const QString &quality, QStringList extraArguments)
{
    auto app = getApp();

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

    bool res = QProcess::startDetached(getStreamlinkProgram() + " " + QString(arguments.join(' ')));

    if (!res) {
        showStreamlinkNotFoundError();
    }
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
