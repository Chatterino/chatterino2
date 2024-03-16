#pragma once

#include "ForwardDecl.hpp"

#include <QTimer>
#include <QWidget>

#include <memory>

namespace chatterino {

class Split;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class AttachedWindow : public QWidget
{
    AttachedWindow(void *_target, int _yOffset);

public:
    struct GetArgs {
        QString winId;
        int yOffset = -1;
        double x = -1;
        double pixelRatio = -1;
        int width = -1;
        int height = -1;
        bool fullscreen = false;
    };

    ~AttachedWindow() override;

    static AttachedWindow *get(void *target_, const GetArgs &args);
#ifdef USEWINSDK
    static AttachedWindow *getForeground(const GetArgs &args);
#endif
    static void detach(const QString &winId);

    void setChannel(ChannelPtr channel);

protected:
    void showEvent(QShowEvent *) override;
    //    virtual void nativeEvent(const QByteArray &eventType, void *message,
    //    long *result) override;

private:
    struct {
        Split *split;
    } ui_{};

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
    int currentYOffset_{};
    double x_ = -1;
    double pixelRatio_ = -1;
    int width_ = 360;
    int height_ = -1;
    bool fullscreen_ = false;

#ifdef USEWINSDK
    bool validProcessName_ = false;
    bool attached_ = false;
#endif
    QTimer timer_;
    QTimer slowTimer_;
};

}  // namespace chatterino
