#pragma once

#include <QProcess>
#include <QScreen>
#include <QWidget>
#include <QWindow>

#include <Application.hpp>
#include <singletons/WindowManager.hpp>
#include <widgets/Window.hpp>

namespace chatterino {

class AttachedPlayer : public QObject
{
    Q_OBJECT
public:
    // Our singleton method
    static AttachedPlayer &getInstance()
    {
        static AttachedPlayer instance;
        return instance;
    }

    // Gets if the stream is currently active or not
    bool getIfStreamActive();

    // Will try to close the stream and player
    void closeStreamThread();

    // Gets the last shown quality setting used
    QString getLastQualitySetting();

    // This will open the attached player and play a given stream
    // If it is not open then this will try to open the window
    void updateStreamLinkProcess(const QString &channel, const QString &quality,
                                 const QString &command);

private slots:

    // This is a callback that will terminate our window threads.
    // VLC doesn't close, so we need to manually kill to ensure we don't run in the background.
    void widgetDestroyed(QObject *widget)
    {
        AttachedPlayer::getInstance().closeStreamThread();
    }

private:
    // singleton cannot be created or copied
    AttachedPlayer()
    {
    }
    AttachedPlayer(AttachedPlayer const &);
    void operator=(AttachedPlayer const &);

    QWindow *attachedWindow = nullptr;
    QWidget *attachedContainer = nullptr;
    unsigned long containerWID;
    QString lastShownChannel = "";
    QString lastQuality = "";
    QProcess *streamlinkProcess = nullptr;
};

}  // namespace chatterino
