#pragma once

#include "messages/MessageThread.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitInput.hpp"

#include <memory>

namespace chatterino {

class ReplyInput : public SplitInput
{
    Q_OBJECT

public:
    ReplyInput(QWidget *parent, Split *split);

    void setThread(const std::shared_ptr<const MessageThread> &thread);

private:
    virtual QString hotkeySendMessage(std::vector<QString> &arguments) override;

    std::shared_ptr<const MessageThread> thread_;
};

};  // namespace chatterino
