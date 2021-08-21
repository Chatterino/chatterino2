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

    const char *getDefaultBinaryPath()
    {
#ifdef _WIN32
        return "C:\\Program Files (x86)\\Streamlink\\bin\\streamlink.exe";
#else
        return "/usr/bin/streamlink";
#endif
    }

    bool checkStreamlinkPath(const QString &path)
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
                    QStringList extraArguments)
{
    auto proc = createStreamlinkProcess();
    auto arguments = proc->arguments()
                     << extraArguments << channelURL << quality;

    // Remove empty arguments before appending additional streamlink options
    // as the options might purposely contain empty arguments
    arguments.removeAll(QString());

    QString additionalOptions = getSettings()->streamlinkOpts.getValue();
    arguments << splitCommand(additionalOptions);

    proc->setArguments(std::move(arguments));
    bool res = proc->startDetached();

    if (!res)
    {
        showStreamlinkNotFoundError();
    }
}

void openStreamlinkForChannel(const QString &channel)
{
    QString channelURL = "twitch.tv/" + channel;

    QString preferredQuality = getSettings()->preferredQuality.getValue();
    preferredQuality = preferredQuality.toLower();

    if (preferredQuality == "choose")
    {
        getStreamQualities(channelURL, [=](QStringList qualityOptions) {
            QualityPopup::showDialog(channelURL, qualityOptions);
        });

        return;
    }

    QStringList args;

    // Quality converted from Chatterino format to Streamlink format
    QString quality;
    // Streamlink qualities to exclude
    QString exclude;

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

    openStreamlink(channelURL, quality, args);
}

}  // namespace chatterino
