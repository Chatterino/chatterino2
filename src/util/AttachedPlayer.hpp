#pragma once

#include <QScreen>
#include <QWidget>
#include <QWindow>

#include <Application.hpp>
#include <singletons/WindowManager.hpp>
#include <widgets/Window.hpp>

namespace chatterino {

class AttachedPlayer
{
public:
    // Our singleton method
    static AttachedPlayer &getInstance()
    {
        static AttachedPlayer instance;
        return instance;
    }

    // Gets if the stream is currently active or not
    bool getIfStreamActive();

    // Gets the last shown quality setting used
    QString getLastQualitySetting();

    // This will open the attached player and play a given stream
    // If it is not open then this will try to open the window
    void updateStreamLinkProcess(const QString &channel, const QString &quality,
                                 const QString &command);

private:
    // singleton cannot be created or copied
    AttachedPlayer()
    {
    }
    AttachedPlayer(AttachedPlayer const &);
    void operator=(AttachedPlayer const &);

    QWindow *mpvWindow = nullptr;
    QWidget *mpvContainer = nullptr;
    unsigned long mpvContainerWID;
    QString lastShownChannel = "";
    QString lastQuality = "";
    QProcess *streamlinkProcess = nullptr;
};

}  // namespace chatterino
