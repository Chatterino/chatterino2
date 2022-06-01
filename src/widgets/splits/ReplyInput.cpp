#include "ReplyInput.hpp"

#include "Application.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/MessageThread.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"
#include "widgets/splits/Split.hpp"

namespace chatterino {

ReplyInput::ReplyInput(QWidget *parent, Split *split)
    : SplitInput(parent, split)
    , thread_(nullptr)
{
}

void ReplyInput::setThread(const std::shared_ptr<const MessageThread> &thread)
{
    this->thread_ = thread;
}

QString ReplyInput::hotkeySendMessage(std::vector<QString> &arguments)
{
    auto c = this->split_->getChannel();
    if (c == nullptr)
        return "";

    if (!c->isTwitchChannel() || this->thread_ == nullptr)
    {
        return SplitInput::hotkeySendMessage(arguments);
    }

    auto tc = dynamic_cast<TwitchChannel *>(c.get());
    if (!tc)
    {
        // this should not fail
        return "";
    }

    QString message = this->ui_.textEdit->toPlainText();

    message = message.replace('\n', ' ');
    QString sendMessage = getApp()->commands->execCommand(message, c, false);

    // Reply within TwitchChannel
    tc->sendReply(sendMessage, this->thread_->rootId());

    this->postMessageSend(message, arguments);
    return "";
}

void ReplyInput::setPlaceholderText(const QString &text)
{
    this->ui_.textEdit->setPlaceholderText(text);
}

}  // namespace chatterino
