#include "util/StreamLink.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/SplitCommand.hpp"
#include "widgets/dialogs/QualityPopup.hpp"

#include <QErrorMessage>
#include <QFileInfo>
#include <QProcess>
#include "common/QLogging.hpp"

#include <functional>

namespace chatterino {

namespace {

    const char *getBinaryName()
    {
#ifdef _WIN32
        return "streamlink.exe";
#else
        return "streamlink";
#endif
    }

    const char *getBinaryNameMPV()
    {
#ifdef _WIN32
        return "mpv.exe";
#else
        return "mpv";
#endif
    }

    const char *getDefaultBinaryPath()
    {
#ifdef _WIN32
        return "C:\\Program Files (x86)\\Streamlink\\bin\\streamlink.exe";
#else
        return "/usr/bin/streamlink";
#endif
    }

    QString getStreamlinkProgram()
    {
        if (getSettings()->streamlinkUseCustomPath)
        {
            return getSettings()->streamlinkPath + "/" + getBinaryName();
        }
        else
        {
            return getBinaryName();
        }
    }

    QString getMPVProgram()
    {
        return getSettings()->mpvPlayerPath + "/" + getBinaryNameMPV();
    }

    bool checkExecutablePath(const QString &path)
    {
        QFileInfo fileinfo(path);

        if (!fileinfo.exists())
        {
            return false;
            // throw Exception(fS("Streamlink path ({}) is invalid, file does
            // not exist", path));
        }

        return fileinfo.isExecutable();
    }

    void showStreamlinkNotFoundError()
    {
        static QErrorMessage *msg = new QErrorMessage;
        msg->setWindowTitle("Chatterino - streamlink not found");

        if (getSettings()->streamlinkUseCustomPath)
        {
            msg->showMessage("Unable to find Streamlink executable\nMake sure "
                             "your custom path is pointing to the DIRECTORY "
                             "where the streamlink executable is located");
        }
        else
        {
            msg->showMessage(
                "Unable to find Streamlink executable.\nIf you have Streamlink "
                "installed, you might need to enable the custom path option");
        }
    }

    void showMPVNotFoundError()
    {
        static QErrorMessage *msg = new QErrorMessage;
        msg->setWindowTitle("Chatterino - mpv player not found");
        msg->showMessage("Unable to find mpv player executable\nMake sure "
                         "your path is pointing to the DIRECTORY "
                         "where the mpv player executable is located");
    }

    QProcess *createStreamlinkProcess()
    {
        auto p = new QProcess;
        p->setProgram(getStreamlinkProgram());

        QObject::connect(p, &QProcess::errorOccurred, [=](auto err) {
            if (err == QProcess::FailedToStart)
            {
                showStreamlinkNotFoundError();
            }
            else
            {
                qCWarning(chatterinoStreamlink) << "Error occurred" << err;
            }

            p->deleteLater();
        });

        QObject::connect(
            p,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
                &QProcess::finished),
            [=](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/) {
                p->deleteLater();
            });

        return p;
    }

}  // namespace

void getStreamQualities(const QString &channelURL,
                        std::function<void(QStringList)> cb)
{
    auto p = createStreamlinkProcess();

    QObject::connect(
        p,
        static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(
            &QProcess::finished),
        [=](int exitCode, QProcess::ExitStatus /*exitStatus*/) {
            if (exitCode != 0)
            {
                qCWarning(chatterinoStreamlink) << "Got error code" << exitCode;
                // return;
            }
            QString lastLine = QString(p->readAllStandardOutput());
            lastLine = lastLine.trimmed().split('\n').last().trimmed();
            if (lastLine.startsWith("Available streams: "))
            {
                QStringList options;
                QStringList split =
                    lastLine.right(lastLine.length() - 19).split(", ");

                for (int i = split.length() - 1; i >= 0; i--)
                {
                    QString option = split.at(i);
                    if (option == "best)")
                    {
                        // As it turns out, sometimes, one quality option can
                        // be the best and worst quality at the same time.
                        // Since we start loop from the end, we can check
                        // that and act accordingly
                        option = split.at(--i);
                        // "900p60 (worst"
                        options << option.left(option.length() - 7);
                    }
                    else if (option.endsWith(" (worst)"))
                    {
                        options << option.left(option.length() - 8);
                    }
                    else if (option.endsWith(" (best)"))
                    {
                        options << option.left(option.length() - 7);
                    }
                    else
                    {
                        options << option;
                    }
                }

                cb(options);
            }
        });

    p->setArguments({channelURL, "--default-stream=KKona"});

    p->start();
}

void openStreamlink(const QString &channelURL, const QString &quality,
                    QStringList extraArguments, bool streamMPV)
{
    QStringList arguments = extraArguments << channelURL << quality;

    // Remove empty arguments before appending additional streamlink options
    // as the options might purposely contain empty arguments
    arguments.removeAll(QString());

    QString additionalOptions = getSettings()->streamlinkOpts.getValue();
    arguments << splitCommand(additionalOptions);

    // If we are not doing our MPV video view start as detached
    // Else we will kill our existing stream proccess and start a new stream
    if (!streamMPV)
    {
        bool res = QProcess::startDetached(getStreamlinkProgram(), arguments);
        if (!res)
        {
            showStreamlinkNotFoundError();
        }
    }
    else
    {
        QString command =
            "\"" + getStreamlinkProgram() + "\" " + arguments.join(" ");
        AttachedPlayer::getInstance().updateStreamLinkProcess(channelURL,
                                                              quality, command);
    }
}

void openStreamlinkForChannel(const QString &channel, bool streamMPV)
{
    QString channelURL = "twitch.tv/" + channel;

    QStringList args;

    // First check to see if player is valid path!
    if (streamMPV && !checkExecutablePath(getMPVProgram()))
    {
        showMPVNotFoundError();
        return;
    }
    if (!checkExecutablePath(getStreamlinkProgram()))
    {
        showStreamlinkNotFoundError();
        return;
    }

    // Append MVP player settings if we have a container to play in
    // https://github.com/mpv-player/mpv/blob/master/DOCS/man/options.rst
    // https://mpv.io/manual/master/#options-wid
    if (streamMPV)
    {
        args << "--player \"" + getMPVProgram() + " --wid=WID\"";
    }

    // Append any extra options to to our stream link command
    // NOTE: it is important to append this before we ask for the quality
    if (getSettings()->streamlinkOptsLatency)
    {
        args << "--twitch-low-latency";
    }
    if (getSettings()->streamlinkOptsAds)
    {
        args << "--twitch-disable-ads";
    }

    // Quality converted from Chatterino format to Streamlink format
    QString quality;
    // Streamlink qualities to exclude
    QString exclude;

    // Check to see if we should ask the user for a quality setting
    // NOTE: if we are using the mpv player, then we should only ask the first time
    // NOTE: afterwards we should just use the last requested quality or a near one
    QString preferredQuality = getSettings()->preferredQuality.getValue();
    preferredQuality = preferredQuality.toLower();
    if (preferredQuality == "choose" && streamMPV &&
        AttachedPlayer::getInstance().getLastQualitySetting() != "")
    {
        args << "--stream-sorting-excludes"
             << ">" + AttachedPlayer::getInstance().getLastQualitySetting();
        quality = "best";
        openStreamlink(channelURL, quality, args, streamMPV);
        return;
    }
    if (preferredQuality == "choose")
    {
        getStreamQualities(channelURL, [=](QStringList qualityOptions) {
            QualityPopup::showDialog(channel, qualityOptions, args, streamMPV);
        });
        return;
    }

    // Else we can set the default
    if (preferredQuality == "high")
    {
        exclude = ">720p30";
        quality = "high,best";
    }
    else if (preferredQuality == "medium")
    {
        exclude = ">540p30";
        quality = "medium,best";
    }
    else if (preferredQuality == "low")
    {
        exclude = ">360p30";
        quality = "low,best";
    }
    else if (preferredQuality == "audio only")
    {
        quality = "audio,audio_only";
    }
    else
    {
        quality = "best";
    }
    if (!exclude.isEmpty())
    {
        args << "--stream-sorting-excludes" << exclude;
    }

    openStreamlink(channelURL, quality, args, streamMPV);
}

}  // namespace chatterino
