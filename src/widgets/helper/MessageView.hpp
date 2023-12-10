#pragma once

#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/Message.hpp"
#include "messages/MessageElement.hpp"
#include "widgets/BaseWidget.hpp"

#include <QWidget>

namespace chatterino {

class MessageLayout;

/// MessageView is a fixed-width widget that displays a single message.
/// For the message to be rendered, you must call setWidth.
class MessageView : public BaseWidget
{
    Q_OBJECT

public:
    MessageView();
    MessageView(MessagePtr message);

    ~MessageView() override;

    void setMessage(MessagePtr message);
    void clearMessage();

    void setWidth(int width);

protected:
    void paintEvent(QPaintEvent * /*event*/) override;
    void themeChangedEvent() override;
    void scaleChangedEvent(float /*newScale*/) override;

private:
    void createMessageLayout();
    void maybeUpdate();
    void layoutMessage();

    MessageElementFlags getFlags() const;

private:
    MessagePtr message_;
    std::unique_ptr<MessageLayout> messageLayout_;

    MessageColors messageColors_;
    MessagePreferences messagePreferences_;

    int width_;
};

}  // namespace chatterino
