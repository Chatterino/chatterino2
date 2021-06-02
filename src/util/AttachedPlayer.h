#pragma once

#include <QWindow>
#include <QWidget>

namespace chatterino {

class AttachedPlayer {
public:

    // Our singleton method
    static AttachedPlayer& getInstance()
    {
        static AttachedPlayer instance;
        return instance;
    }

    // Gets if the stream is currently active or not
    bool getIfStreamActive();

    // This will open the attached player and play a given stream
    // If it is not open then this will try to open the window
    void updateStreamLinkProcess(const QString &channel, const QString &command);

private:

    // singleton cannot be created or copied
    AttachedPlayer() {}
    AttachedPlayer(AttachedPlayer const&);
    void operator=(AttachedPlayer const&);

    // TODO: rename these to be better
    QWindow *mpvWindow = nullptr;
    QWidget *mpvContainer = nullptr;
    unsigned long mpvContainerWID;
    QString lastShownChannel = "";
    QProcess* streamlinkProcess = nullptr;


};


}  // namespace chatterino
