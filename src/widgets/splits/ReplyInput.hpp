#pragma once

#include "widgets/splits/SplitInput.hpp"

#include <memory>

namespace chatterino {

class MessageThread;
class Split;

class ReplyInput : public SplitInput
{
    Q_OBJECT

public:
    ReplyInput(QWidget *parent, Split *split);

    void setThread(const std::shared_ptr<const MessageThread> &thread);

    void setPlaceholderText(const QString &text);

private:
    virtual QString hotkeySendMessage(std::vector<QString> &arguments) override;

    std::shared_ptr<const MessageThread> thread_;
};

};  // namespace chatterino
