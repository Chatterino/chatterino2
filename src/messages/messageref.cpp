#include "messageref.hpp"
#include "emotemanager.hpp"
#include "settingsmanager.hpp"

#include <QDebug>

#define MARGIN_LEFT 8
#define MARGIN_RIGHT 8
#define MARGIN_TOP 4
#define MARGIN_BOTTOM 4

using namespace chatterino::messages;

namespace chatterino {
namespace messages {

MessageRef::MessageRef(SharedMessage _message)
    : message(_message)
    , wordParts()
{
}

Message *MessageRef::getMessage()
{
    return this->message.get();
}

int MessageRef::getHeight() const
{
    return this->height;
}

bool MessageRef::layout(int width, bool enableEmoteMargins)
{
    auto &settings = SettingsManager::getInstance();

    bool sizeChanged = width != this->currentLayoutWidth;
    bool redraw = width != this->currentLayoutWidth;
    int spaceWidth = 4;

    int mediumTextLineHeight =
        FontManager::getInstance().getFontMetrics(FontManager::Medium).height();

    /* TODO(pajlada): Re-implement
    bool recalculateImages = this->emoteGeneration != EmoteManager::getInstance().getGeneration();
    */
    bool recalculateImages = true;

    bool recalculateText = this->fontGeneration != FontManager::getInstance().getGeneration();
    bool newWordTypes = this->currentWordTypes != SettingsManager::getInstance().getWordTypeMask();

    qreal emoteScale = settings.emoteScale.get();
    bool scaleEmotesByLineHeight = settings.scaleEmotesByLineHeight.get();

    // calculate word sizes
    if (!redraw && !recalculateImages && !recalculateText && !newWordTypes) {
        return false;
    }

    // this->emoteGeneration = EmoteManager::getInstance().getGeneration();
    this->fontGeneration = FontManager::getInstance().getGeneration();

    for (auto &word : this->message->getWords()) {
        if (word.isImage()) {
            if (!recalculateImages) {
                continue;
            }

            auto &image = word.getImage();

            qreal w = image.getWidth();
            qreal h = image.getHeight();

            if (scaleEmotesByLineHeight) {
                word.setSize(w * mediumTextLineHeight / h * emoteScale,
                             mediumTextLineHeight * emoteScale);
            } else {
                word.setSize(w * image.getScale() * emoteScale, h * image.getScale() * emoteScale);
            }
        } else {
            if (!recalculateText) {
                continue;
            }

            QFontMetrics &metrics = word.getFontMetrics();
            word.setSize(metrics.width(word.getText()), metrics.height());
        }
    }

    if (newWordTypes) {
        this->currentWordTypes = settings.getWordTypeMask();
    }

    // layout
    this->currentLayoutWidth = width;

    int x = MARGIN_LEFT;
    int y = MARGIN_TOP;

    int right = width - MARGIN_RIGHT;

    int lineNumber = 0;
    int lineStart = 0;
    int lineHeight = 0;
    bool first = true;

    this->wordParts.clear();

    uint32_t flags = settings.getWordTypeMask();

    for (auto it = this->message->getWords().begin(); it != this->message->getWords().end(); ++it) {
        Word &word = *it;

        // Check if given word is supposed to be rendered by comparing it to the current setting
        if ((word.getType() & flags) == Word::None) {
            continue;
        }

        int xOffset = 0, yOffset = 0;

        if (enableEmoteMargins) {
            if (word.isImage() && word.getImage().isHat()) {
                xOffset = -word.getWidth() + 2;
            } else {
                xOffset = word.getXOffset();
                yOffset = word.getYOffset();
            }
        }

        // word wrapping
        if (word.isText() && word.getWidth() + MARGIN_LEFT > right) {
            alignWordParts(lineStart, lineHeight, width);

            y += lineHeight;

            const QString &text = word.getText();

            int start = 0;
            QFontMetrics &metrics = word.getFontMetrics();

            int width = 0;

            std::vector<short> &charWidths = word.getCharacterWidthCache();
            int charOffset = 0;

            for (int i = 2; i <= text.length(); i++) {
                if ((width = width + charWidths[i - 1]) + MARGIN_LEFT > right) {
                    QString mid = text.mid(start, i - start - 1);

                    this->wordParts.push_back(WordPart(word, MARGIN_LEFT, y, width,
                                                       word.getHeight(), lineNumber, mid, mid,
                                                       false, charOffset));

                    charOffset = i;

                    y += metrics.height();

                    start = i - 1;

                    width = 0;
                    lineNumber++;
                }
            }

            QString mid(text.mid(start));
            width = metrics.width(mid);

            this->wordParts.push_back(WordPart(word, MARGIN_LEFT, y - word.getHeight(), width,
                                               word.getHeight(), lineNumber, mid, mid, charOffset));
            x = width + MARGIN_LEFT + spaceWidth;

            lineHeight = word.getHeight();

            lineStart = this->wordParts.size() - 1;

            first = false;
        } else if (first || x + word.getWidth() + xOffset <= right) {
            // fits in the line
            this->wordParts.push_back(
                WordPart(word, x, y - word.getHeight(), lineNumber, word.getCopyText()));

            x += word.getWidth() + xOffset;
            x += spaceWidth;

            lineHeight = std::max(word.getHeight(), lineHeight);

            first = false;
        } else {
            // doesn't fit in the line
            alignWordParts(lineStart, lineHeight, width);

            y += lineHeight;

            lineNumber++;

            this->wordParts.push_back(
                WordPart(word, MARGIN_LEFT, y - word.getHeight(), lineNumber, word.getCopyText()));

            lineStart = this->wordParts.size() - 1;

            lineHeight = word.getHeight();

            x = word.getWidth() + MARGIN_LEFT;
            x += spaceWidth;
        }
    }

    alignWordParts(lineStart, lineHeight, width);

    if (this->height != y + lineHeight) {
        sizeChanged = true;
        this->height = y + lineHeight;
    }

    this->height += MARGIN_BOTTOM;

    if (sizeChanged) {
        buffer = nullptr;
    }

    updateBuffer = true;

    return true;
}

const std::vector<WordPart> &MessageRef::getWordParts() const
{
    return this->wordParts;
}

void MessageRef::alignWordParts(int lineStart, int lineHeight, int width)
{
    int xOffset = 0;

    if (this->message->centered && this->wordParts.size() > 0) {
        xOffset = (width - this->wordParts.at(this->wordParts.size() - 1).getRight()) / 2;
    }

    for (size_t i = lineStart; i < this->wordParts.size(); i++) {
        WordPart &wordPart2 = this->wordParts.at(i);

        wordPart2.setPosition(wordPart2.getX() + xOffset, wordPart2.getY() + lineHeight);
    }
}

const Word *MessageRef::tryGetWordPart(QPoint point)
{
    // go through all words and return the first one that contains the point.
    for (WordPart &wordPart : this->wordParts) {
        if (wordPart.getRect().contains(point)) {
            return &wordPart.getWord();
        }
    }

    return nullptr;
}

int MessageRef::getSelectionIndex(QPoint position)
{
    if (this->wordParts.size() == 0) {
        return 0;
    }

    // find out in which line the cursor is
    int lineNumber = 0, lineStart = 0, lineEnd = 0;

    for (size_t i = 0; i < this->wordParts.size(); i++) {
        WordPart &part = this->wordParts[i];

        if (part.getLineNumber() != 0 && position.y() < part.getY()) {
            break;
        }

        if (part.getLineNumber() != lineNumber) {
            lineStart = i;
            lineNumber = part.getLineNumber();
        }

        lineEnd = i + 1;
    }

    // count up to the cursor
    int index = 0;

    for (int i = 0; i < lineStart; i++) {
        WordPart &part = this->wordParts[i];

        index += part.getWord().isImage() ? 2 : part.getText().length() + 1;
    }

    for (int i = lineStart; i < lineEnd; i++) {
        WordPart &part = this->wordParts[i];

        // curser is left of the word part
        if (position.x() < part.getX()) {
            break;
        }

        // cursor is right of the word part
        if (position.x() > part.getX() + part.getWidth()) {
            index += part.getCharacterLength();
            continue;
        }

        // cursor is over the word part
        if (part.getWord().isImage()) {
            if (position.x() - part.getX() > part.getWidth() / 2) {
                index++;
            }
        } else {
            // TODO: use word.getCharacterWidthCache();

            auto text = part.getText();

            int x = part.getX();

            for (int j = 0; j < text.length(); j++) {
                if (x > position.x()) {
                    break;
                }

                index++;
                x = part.getX() + part.getWord().getFontMetrics().width(text, j + 1);
            }
        }

        break;
    }

    return index;
}

}  // namespace messages
}  // namespace chatterino
