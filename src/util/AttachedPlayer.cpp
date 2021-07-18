#include "AttachedPlayer.hpp"

namespace chatterino {

bool AttachedPlayer::getIfStreamActive()
{
    return !(mpvWindow == nullptr || mpvContainer == nullptr ||
             !mpvContainer->isVisible());
}

QString AttachedPlayer::getLastQualitySetting()
{
    if (!getIfStreamActive())
    {
        lastQuality = "";
    }
    return lastQuality;
}

void AttachedPlayer::updateStreamLinkProcess(const QString &channel,
                                             const QString &quality,
                                             const QString &command)
{
    // Return doing nothing if the stream is already active and it is the same channel
    if (getIfStreamActive() && channel == lastShownChannel)
    {
        return;
    }

    // Create the attached window we will stream the video into
    // TODO: we should try to dock this to the application main window??
    // TODO: how to add a volume control to this??
    if (!getIfStreamActive())
    {
        // create the window
        QWindow *mainWindow =
            chatterino::getApp()->windows->getMainWindow().windowHandle();
        mpvWindow = new QWindow;
        //mpvWindow = mainWindow;
        mpvContainerWID = mpvWindow->winId();
        mpvContainer = QWidget::createWindowContainer(mpvWindow);
        mpvContainer->setBackgroundRole(QPalette::Window);
        mpvContainer->setSizePolicy(QSizePolicy::Policy::Expanding,
                                    QSizePolicy::Policy::Expanding);
        // the size should default to near the same height as the main chat client
        // it should also not overflow the monitor, so also compare the width
        int maxWidth = mainWindow->screen()->geometry().width() -
                       mainWindow->size().width();
        int height = mainWindow->size().height();
        int width = (int)qMin((float)maxWidth, 16.0f / 9.0f * (float)height);
        height = (int)(9.0f / 16.0f * (float)width);
        mpvContainer->resize(width, height);
        mpvContainer->show();
    }

    // Now we need to replace any window ids in the command
    // https://mpv.io/
    // https://github.com/mpv-player/mpv/blob/master/DOCS/man/options.rst
    QString commandEdited = command;
    commandEdited.replace("WID", QString::number(mpvContainerWID));

    // Now lets update our process
    if (streamlinkProcess != nullptr)
    {
        streamlinkProcess->terminate();
        streamlinkProcess->kill();
    }
    streamlinkProcess = new QProcess;
    streamlinkProcess->start(commandEdited);

    // Update the last channel we displayed
    lastShownChannel = channel;
    if (lastQuality == "")
    {
        lastQuality = quality;
    }
}
}  // namespace chatterino
