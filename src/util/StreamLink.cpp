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
#include "common/Version.hpp"

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

    const char *getBinaryNameVLC()
    {
#ifdef _WIN32
        return "vlc.exe";
#else
        return "vlc";
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

    QString getVLCProgram()
    {
        return getSettings()->vlcPlayerPath + "/" + getBinaryNameVLC();
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

    void showVLCNotFoundError()
    {
        static QErrorMessage *msg = new QErrorMessage;
        msg->setWindowTitle("Chatterino - VLC player not found");
        msg->showMessage("Unable to find VLC player executable\nMake sure "
                         "your path is pointing to the DIRECTORY "
                         "where the VLC player executable is located");
    }

    QProcess *createStreamlinkProcess()
    {
        auto p = new QProcess;

        const QString path = [] {
            if (getSettings()->streamlinkUseCustomPath)
            {
                return getSettings()->streamlinkPath + "/" + getBinaryName();
            }
            else
            {
                return QString{getBinaryName()};
            }
        }();

        if (Version::instance().isFlatpak())
        {
            p->setProgram("flatpak-spawn");
            p->setArguments({"--host", path});
        }
        else
        {
            p->setProgram(path);
        }

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

    p->setArguments(p->arguments() +
                    QStringList{channelURL, "--default-stream=KKona"});

    p->start();
}

void openStreamlink(const QString &channelURL, const QString &quality,
                    QStringList extraArguments, bool streamVLC)
{
    auto proc = createStreamlinkProcess();
    auto arguments = proc->arguments()
                     << extraArguments << channelURL << quality;

    // Remove empty arguments before appending additional streamlink options
    // as the options might purposely contain empty arguments
    arguments.removeAll(QString());

    QString additionalOptions = getSettings()->streamlinkOpts.getValue();
    arguments << splitCommand(additionalOptions);

    // If we are not doing our VLC video view start as detached
    // Else we will kill our existing stream proccess and start a new stream
    if (!streamVLC)
    {
        proc->setArguments(std::move(arguments));
        bool res = proc->startDetached();
        if (!res)
        {
            showStreamlinkNotFoundError();
        }
    }
    else
    {
        // TODO: use the createStreamlinkProcess() here also...
        // TODO: right now we just kill the created process...
        proc->terminate();
        QString command =
            "\"" + getStreamlinkProgram() + "\" " + arguments.join(" ");
        AttachedPlayer::getInstance().updateStreamLinkProcess(channelURL,
                                                              quality, command);
    }
}

void openStreamlinkForChannel(const QString &channel, bool streamVLC)
{
    QString channelURL = "twitch.tv/" + channel;

    QStringList args;

    // First check to see if player is valid path!
    if (streamVLC && !checkExecutablePath(getVLCProgram()))
    {
        showVLCNotFoundError();
        return;
    }
    if (!checkExecutablePath(getStreamlinkProgram()))
    {
        showStreamlinkNotFoundError();
        return;
    }

    // Append VLC player settings if we have a container to play in
    // https://wiki.videolan.org/VLC_command-line_help/
    if (streamVLC)
    {
        args
            << "--player \"" + getVLCProgram() +
                   " --play-and-exit -I dummy --no-embedded-video "
                   "--qt-notification=0 --qt-auto-raise=0 --qt-start-minimized "
                   "--no-qt-name-in-title --no-video-title-show "
                   "--drawable-hwnd=WID\"";
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
    // NOTE: if we are using the VLC player, then we should only ask the first time
    // NOTE: afterwards we should just use the last requested quality or a near one
    QString preferredQuality = getSettings()->preferredQuality.getValue();
    preferredQuality = preferredQuality.toLower();
    if (preferredQuality == "choose" && streamVLC &&
        AttachedPlayer::getInstance().getLastQualitySetting() != "")
    {
        args << "--stream-sorting-excludes"
             << ">" + AttachedPlayer::getInstance().getLastQualitySetting();
        quality = "best";
        openStreamlink(channelURL, quality, args, streamVLC);
        return;
    }
    if (preferredQuality == "choose")
    {
        getStreamQualities(channelURL, [=](QStringList qualityOptions) {
            QualityPopup::showDialog(channelURL, qualityOptions, args,
                                     streamVLC);
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

    openStreamlink(channelURL, quality, args, streamVLC);
}

}  // namespace chatterino
