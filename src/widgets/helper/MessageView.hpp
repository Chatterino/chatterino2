#pragma once

#include "messages/Message.hpp"
#include "widgets/BaseWidget.hpp"

#include <QWidget>

namespace chatterino {

class MessageLayout;

class MessageView : public BaseWidget
{
    Q_OBJECT

public:
    MessageView();
    MessageView(MessagePtr message);

    ~MessageView();

    void setMessage(MessagePtr message);
    void clearMessage();

    void setWidth(int width);

protected:
    void paintEvent(QPaintEvent * /*event*/) override;

private:
    void createMessageLayout();
    void maybeUpdate();
    void layoutMessage();

private:
    MessagePtr message_;
    std::unique_ptr<MessageLayout> messageLayout_;

    int width_;
};

}  // namespace chatterino
