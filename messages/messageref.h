#ifndef MESSAGEREF_H
#define MESSAGEREF_H

#include "messages/message.h"

#include <QPixmap>
#include <memory>

namespace chatterino {
namespace messages {

class MessageRef
{
public:
    MessageRef(std::shared_ptr<Message> message);

    Message *
    getMessage()
    {
        return this->message;
    }

    int
    getHeight() const
    {
        return height;
    }

    bool layout(int width, bool enableEmoteMargins = true);

    const std::vector<WordPart> &
    getWordParts() const
    {
        return wordParts;
    }

    std::shared_ptr<QPixmap> buffer = nullptr;
    bool updateBuffer = false;

private:
    Message *message;
    std::shared_ptr<Message> messagePtr;

    std::vector<messages::WordPart> wordParts;

    int height = 0;

    int currentLayoutWidth = -1;
    int fontGeneration = -1;
    int emoteGeneration = -1;

    void alignWordParts(int lineStart, int lineHeight);
};
}
}

#endif  // MESSAGEREF_H
