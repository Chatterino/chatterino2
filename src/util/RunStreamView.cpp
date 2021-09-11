#include "RunStreamView.hpp"

#include <QFile>

#ifdef USEWINSDK
#    include <Windows.h>
#    include "widgets/FramelessEmbedWindow.hpp"
#endif

namespace chatterino {

namespace {
    QString streamViewPath()
    {
        return qApp->applicationDirPath() + "/StreamView/StreamView.exe";
        // return R"(C:\Users\daniel\Documents\Git\chatterino-streamview\StreamView\StreamView\bin\Debug\StreamView.exe)";
    }
}  // namespace

bool canRunStreamView()
{
#ifdef USEWINSDK
    static bool value = QFile(streamViewPath()).exists();
    return value;
#else
    return false;
#endif
}

void runStreamView(const QString &channel)
{
#ifdef USEWINSDK
    FramelessEmbedWindow *window = new FramelessEmbedWindow;

    window->createWinId();
    auto chatHandle = QString::number(window->winId());

    QProcess::startDetached(
        streamViewPath(), {"--chat-handle", chatHandle, "--channel", channel});

    // Dispose self if there's no parent after 30 seconds
    QTimer::singleShot(30000, window, [window] {
        if (GetParent(reinterpret_cast<HWND>(window->winId())) == nullptr)
        {
            window->close();
        }
    });
#endif
}

}  // namespace chatterino
