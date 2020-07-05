#pragma once

#include "messages/MessageElement.hpp"

#include <QRegularExpression>
#include <ctime>
#include <utility>

namespace chatterino {
struct BanAction;
struct UnbanAction;
struct AutomodAction;
struct AutomodUserAction;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;

struct SystemMessageTag {
};
struct TimeoutMessageTag {
};
const SystemMessageTag systemMessage{};
const TimeoutMessageTag timeoutMessage{};

MessagePtr makeSystemMessage(const QString &text);
MessagePtr makeSystemMessage(const QString &text, const QTime &time);
std::pair<MessagePtr, MessagePtr> makeAutomodMessage(
    const AutomodAction &action);

struct MessageParseArgs {
    bool disablePingSounds = false;
    bool isReceivedWhisper = false;
    bool isSentWhisper = false;
    bool trimSubscriberUsername = false;
    bool isStaffOrBroadcaster = false;
};

class MessageBuilder

{
public:
    MessageBuilder();
    MessageBuilder(SystemMessageTag, const QString &text,
                   const QTime &time = QTime::currentTime());
    MessageBuilder(TimeoutMessageTag, const QString &systemMessageText,
                   int times);
    MessageBuilder(TimeoutMessageTag, const QString &username,
                   const QString &durationInSeconds, const QString &reason,
                   bool multipleTimes);
    MessageBuilder(const BanAction &action, uint32_t count = 1);
    MessageBuilder(const UnbanAction &action);
    MessageBuilder(const AutomodUserAction &action);

    Message *operator->();
    Message &message();
    MessagePtr release();
    std::weak_ptr<Message> weakOf();

    void append(std::unique_ptr<MessageElement> element);
    QString matchLink(const QString &string);
    void addLink(const QString &origLink, const QString &matchedLink);

    template <typename T, typename... Args>
    T *emplace(Args &&... args)
    {
        static_assert(std::is_base_of<MessageElement, T>::value,
                      "T must extend MessageElement");

        auto unique = std::make_unique<T>(std::forward<Args>(args)...);
        auto pointer = unique.get();
        this->append(std::move(unique));
        return pointer;
    }

private:
    // Helper method that emplaces some text stylized as system text
    // and then appends that text to the QString parameter "toUpdate".
    // Returns the TextElement that was emplaced.
    TextElement *emplaceSystemTextAndUpdate(const QString &text,
                                            QString &toUpdate);

    std::shared_ptr<Message> message_;
};
}  // namespace chatterino
