#include "ReplyInput.hpp"

#include "Application.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "widgets/helper/ResizingTextEdit.hpp"

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

    tc->sendReply(sendMessage, this->thread_->rootId(),
                  this->thread_->root()->loginName);
    // don't add duplicate messages and empty message to message history
    if ((this->prevMsg_.isEmpty() || !this->prevMsg_.endsWith(message)) &&
        !message.trimmed().isEmpty())
    {
        this->prevMsg_.append(message);
    }
    bool shouldClearInput = true;
    if (arguments.size() != 0 && arguments.at(0) == "keepInput")
    {
        shouldClearInput = false;
    }

    if (shouldClearInput)
    {
        this->currMsg_ = QString();
        this->ui_.textEdit->setPlainText(QString());
    }
    this->prevIndex_ = this->prevMsg_.size();
    return "";
}

}  // namespace chatterino
