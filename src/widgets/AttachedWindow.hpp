#pragma once

#include <QWidget>

#include "common/Channel.hpp"
#include "widgets/splits/Split.hpp"

namespace chatterino {

class AttachedWindow : public QWidget
{
    AttachedWindow(void *_target, int _yOffset);

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
    struct {
        Split *split;
    } ui_;

    struct Item {
        void *hwnd;
        AttachedWindow *window;
        QString winId;
    };

    static std::vector<Item> items;

    void attachToHwnd(void *attached);
    void updateWindowRect(void *attached);

    void *target_;
    int yOffset_;
    int currentYOffset_;
    int width_ = 360;
    int height_ = -1;
#ifdef USEWINSDK
    bool validProcessName_ = false;
    bool attached_ = false;
#endif
    QTimer timer_;
};

}  // namespace chatterino
