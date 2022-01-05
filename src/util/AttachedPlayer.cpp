#include "AttachedPlayer.hpp"

namespace chatterino {

bool AttachedPlayer::getIfStreamActive()
{
    return !(attachedWindow == nullptr || attachedContainer == nullptr ||
             !attachedContainer->isVisible());
}

void AttachedPlayer::closeStreamThread()
{
    if (streamlinkProcess != nullptr)
    {
        streamlinkProcess->kill();
        streamlinkProcess->terminate();
    }
    attachedWindow = nullptr;
    attachedContainer = nullptr;
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
    bool isActive = getIfStreamActive();
    if (isActive && channel == lastShownChannel)
    {
        return;
    }

    // Create the attached window we will stream the video into
    // TODO: we should try to dock this to the application main window??
    if (!isActive)
    {
        // create the window
        QWindow *mainWindow =
            chatterino::getApp()->windows->getMainWindow().windowHandle();
        attachedWindow = new QWindow;
        attachedContainer = QWidget::createWindowContainer(attachedWindow);
        attachedContainer->setAttribute(Qt::WA_DontCreateNativeAncestors);
        attachedContainer->setAttribute(Qt::WA_NativeWindow);
        containerWID = attachedContainer->winId();
        attachedContainer->setBackgroundRole(QPalette::Window);
        attachedContainer->setSizePolicy(QSizePolicy::Policy::Expanding,
                                         QSizePolicy::Policy::Expanding);
        attachedContainer->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(attachedContainer, SIGNAL(destroyed(QObject *)), this,
                         SLOT(widgetDestroyed(QObject *)));

        // the size should default to near the same height as the main chat client
        // it should also not overflow the monitor, so also compare the width
        int maxWidth = 0.9 * (mainWindow->screen()->geometry().width() -
                              mainWindow->size().width());
        int height = mainWindow->size().height();
        int width = (int)qMin((float)maxWidth, 16.0f / 9.0f * (float)height);
        height = (int)(9.0f / 16.0f * (float)width);
        attachedContainer->resize(width, height);
        attachedContainer->show();
    }

    // Now we need to replace any window ids in the command
    // https://wiki.videolan.org/VLC_command-line_help/
    QString commandEdited = command;
    commandEdited.replace("WID", QString::number(containerWID));

    // Now lets update our process
    if (streamlinkProcess != nullptr)
    {
        streamlinkProcess->kill();
        streamlinkProcess->terminate();
    }
    streamlinkProcess = new QProcess;
    streamlinkProcess->start(commandEdited);
    qInfo() << "STREAMLINK: " << commandEdited;

    // Update the last channel we displayed
    lastShownChannel = channel;
    if (lastQuality == "")
    {
        lastQuality = quality;
    }
}
}  // namespace chatterino
