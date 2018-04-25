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
    ~AttachedWindow();

    static AttachedWindow *get(void *target, const QString &winId, int yOffset);
    static void detach(const QString &winId);

    void setChannel(ChannelPtr channel);

protected:
    virtual void showEvent(QShowEvent *) override;
    //    virtual void nativeEvent(const QByteArray &eventType, void *message, long *result)
    //    override;

private:
    void *target;
    int yOffset;

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
