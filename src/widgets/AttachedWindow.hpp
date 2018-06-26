#pragma once

#include <QWidget>

#include "Channel.hpp"
#include "widgets/splits/Split.hpp"

namespace chatterino {
namespace widgets {

class AttachedWindow : public QWidget
{
    AttachedWindow(void *target_, int asdf);

public:
    struct GetArgs {
        QString winId;
        int yOffset = -1;
        int width = -1;
        int height = -1;
    };

    virtual ~AttachedWindow() override;

    static AttachedWindow *get(void *target_, const GetArgs &args);
    static void detach(const QString &winId);

    void setChannel(ChannelPtr channel);

protected:
    virtual void showEvent(QShowEvent *) override;
    //    virtual void nativeEvent(const QByteArray &eventType, void *message, long *result)
    //    override;

private:
    void *target_;
    int yOffset_;
    int currentYOffset_;
    int width_ = 360;
    int height_ = -1;
    bool validProcessName_ = false;
    bool attached_ = false;
    QTimer timer_;

    struct {
        Split *split;
    } ui_;

    void attachToHwnd_(void *attached);
    void updateWindowRect_(void *attached);

    struct Item {
        void *hwnd;
        AttachedWindow *window;
        QString winId;
    };

    static std::vector<Item> items;
};

}  // namespace widgets
}  // namespace chatterino
