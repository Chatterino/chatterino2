#pragma once

#include <QWidget>

#include "channel.hpp"
#include "widgets/split.hpp"

namespace chatterino {
namespace widgets {

class AttachedWindow : public QWidget
{
    AttachedWindow(void *target, int asdf);

public:
    struct GetArgs {
        QString winId;
        int yOffset = -1;
        int width = -1;
        int height = -1;
    };

    ~AttachedWindow();

    static AttachedWindow *get(void *target, const GetArgs &args);
    static void detach(const QString &winId);

    void setChannel(ChannelPtr channel);

protected:
    virtual void showEvent(QShowEvent *) override;
    //    virtual void nativeEvent(const QByteArray &eventType, void *message, long *result)
    //    override;

private:
    void *target;
    int yOffset;
    int currentYOffset;
    int _width = 360;
    int _height = -1;

    struct {
        Split *split;
    } ui;

    void attachToHwnd(void *hwnd);

    struct Item {
        void *hwnd;
        AttachedWindow *window;
        QString winId;
    };

    static std::vector<Item> items;
};

}  // namespace widgets
}  // namespace chatterino
