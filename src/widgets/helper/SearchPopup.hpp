#pragma once

#include "ForwardDecl.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "widgets/BaseWindow.hpp"

#include <memory>

class QLineEdit;

namespace chatterino {

class SearchPopup : public BaseWindow
{
public:
    SearchPopup();

    virtual void setChannel(const ChannelPtr &channel);

protected:
    void keyPressEvent(QKeyEvent *e) override;

    virtual void updateWindowTitle();

private:
    void initLayout();
    void search();

    LimitedQueueSnapshot<MessagePtr> snapshot_;
    QLineEdit *searchInput_{};
    ChannelView *channelView_{};
    QString channelName_{};
};

}  // namespace chatterino
