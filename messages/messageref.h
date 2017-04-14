#ifndef MESSAGEREF_H
#define MESSAGEREF_H

#include "messages/message.h"

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

    bool tryGetWordPart(QPoint point, messages::Word &word);

    int getSelectionIndex(QPoint position);

private:
    // variables
    SharedMessage _message;
    std::vector<messages::WordPart> _wordParts;

    int _height = 0;

    int _currentLayoutWidth = -1;
    int _fontGeneration = -1;
    int _emoteGeneration = -1;
    Word::Type _currentWordTypes = Word::None;

    // methods
    void alignWordParts(int lineStart, int lineHeight);
};
}
}

#endif  // MESSAGEREF_H
