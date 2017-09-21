#pragma once

#include "messages/message.hpp"

#include <QPixmap>

#include <memory>

namespace chatterino {
namespace messages {

class MessageRef;

typedef std::shared_ptr<MessageRef> SharedMessageRef;

class MessageRef
{
public:
    MessageRef(SharedMessage message);

    Message *getMessage();
    int getHeight() const;

    bool layout(int width, bool enableEmoteMargins = true);

    const std::vector<WordPart> &getWordParts() const;

    std::shared_ptr<QPixmap> buffer = nullptr;
    bool updateBuffer = false;

    const messages::Word *tryGetWordPart(QPoint point);

    int getSelectionIndex(QPoint position);

private:
    // variables
    SharedMessage message;
    std::vector<messages::WordPart> wordParts;

    int height = 0;

    int currentLayoutWidth = -1;
    int fontGeneration = -1;
    /* TODO(pajlada): Re-implement
    int emoteGeneration = -1;
    */
    Word::Type currentWordTypes = Word::None;

    // methods
    void alignWordParts(int lineStart, int lineHeight, int width);
};

}  // namespace messages
}  // namespace chatterino
