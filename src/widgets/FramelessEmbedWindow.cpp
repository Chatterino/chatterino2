#include "widgets/FramelessEmbedWindow.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "widgets/splits/Split.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QMessageBox>

#ifdef USEWINSDK
#    include "Windows.h"
#endif

namespace chatterino {

FramelessEmbedWindow::FramelessEmbedWindow()
    : BaseWindow({BaseWindow::Frameless, BaseWindow::DisableLayoutSave})
{
    this->split_ = new Split((QWidget *)nullptr);
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(this->split_);

    this->getLayoutContainer()->setLayout(layout);
}

#ifdef USEWINSDK
#    if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool FramelessEmbedWindow::nativeEvent(const QByteArray &eventType,
                                       void *message, qintptr *result)
#    else
bool FramelessEmbedWindow::nativeEvent(const QByteArray &eventType,
                                       void *message, long *result)
#    endif
{
    MSG *msg = reinterpret_cast<MSG *>(message);

    if (msg->message == WM_COPYDATA)
    {
        auto data = reinterpret_cast<COPYDATASTRUCT *>(msg->lParam);

        // no idea why I have to read it to a string and then encode it back to utf-8
        auto str = QString::fromUtf8(reinterpret_cast<char *>(data->lpData),
                                     int(data->cbData));
        auto doc = QJsonDocument::fromJson(str.toUtf8());

        auto root = doc.object();
        if (root.value("type").toString() == "set-channel")
        {
            if (root.value("provider").toString() == "twitch")
            {
                auto channelName = root.value("channel-name").toString();

                this->split_->setChannel(
                    getApp()->getTwitch()->getOrAddChannel(channelName));
            }
        }
    }

    return BaseWidget::nativeEvent(eventType, message, result);
}

void FramelessEmbedWindow::showEvent(QShowEvent *)
{
    if (!getApp()->getArgs().parentWindowId)
    {
        return;
    }

    if (auto parentHwnd =
            reinterpret_cast<HWND>(getApp()->getArgs().parentWindowId.value()))
    {
        auto handle = reinterpret_cast<HWND>(this->winId());
        if (!::SetParent(handle, parentHwnd))
        {
            QApplication::exit(1);
        }

        QJsonDocument doc;
        QJsonObject root;
        root.insert("type", "created-window");
        root.insert(
            "window-id",
            QString::number(reinterpret_cast<unsigned long long>(handle)));
        doc.setObject(root);
        auto json = doc.toJson();
        json.append('\0');

        COPYDATASTRUCT cds;
        cds.cbData = static_cast<DWORD>(json.size());
        cds.lpData = json.data();

        ::SendMessage(parentHwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(handle),
                      reinterpret_cast<LPARAM>(&cds));
    }
}
#endif

}  // namespace chatterino
