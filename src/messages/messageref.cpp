#include "messages/messageref.hpp"
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

// return true if redraw is required
bool MessageRef::layout(int width, float dpiMultiplyer)
{
    bool layoutRequired = false;

    // check if width changed
    bool widthChanged = width != this->currentLayoutWidth;
    layoutRequired |= widthChanged;
    this->currentLayoutWidth = width;

    // check if emotes changed
    bool imagesChanged = this->emoteGeneration != EmoteManager::instance->getGeneration();
    layoutRequired |= imagesChanged;
    this->emoteGeneration = EmoteManager::instance->getGeneration();

    // check if text changed
    bool textChanged = this->fontGeneration != FontManager::getInstance().getGeneration();
    layoutRequired |= textChanged;
    this->fontGeneration = FontManager::getInstance().getGeneration();

    // check if work mask changed
    bool wordMaskChanged =
        this->currentWordTypes != SettingsManager::getInstance().getWordTypeMask();
    layoutRequired |= wordMaskChanged;
    this->currentWordTypes = SettingsManager::getInstance().getWordTypeMask();

    // check if dpi changed
    bool dpiChanged = this->dpiMultiplyer != dpiMultiplyer;
    layoutRequired |= dpiChanged;
    this->dpiMultiplyer = dpiMultiplyer;
    imagesChanged |= dpiChanged;
    textChanged |= dpiChanged;

    // update word sizes if needed
    if (imagesChanged) {
        this->updateImageSizes();
    }
    if (textChanged) {
        this->updateTextSizes();
    }
    if (widthChanged || wordMaskChanged) {
        this->buffer = nullptr;
    }

    // return if no layout is required
    if (!layoutRequired) {
        return false;
    }

    this->actuallyLayout(width);

    return true;
}

void MessageRef::actuallyLayout(int width)
{
    auto &settings = SettingsManager::getInstance();

    const int spaceWidth = 4;
    const int right = width - MARGIN_RIGHT;

    // clear word parts
    this->wordParts.clear();

    // layout
    int x = MARGIN_LEFT;
    int y = MARGIN_TOP;

    int lineNumber = 0;
    int lineStart = 0;
    int lineHeight = 0;
    bool first = true;

    uint32_t flags = settings.getWordTypeMask();

    // loop throught all the words and add them when a line is full
    for (auto it = this->message->getWords().begin(); it != this->message->getWords().end(); ++it) {
        Word &word = *it;

        // Check if given word is supposed to be rendered by comparing it to the current setting
        if ((word.getType() & flags) == Word::None) {
            continue;
        }

        int xOffset = 0, yOffset = 0;

        ///        if (enableEmoteMargins) {
        ///            if (word.isImage() && word.getImage().isHat()) {
        ///                xOffset = -word.getWidth() + 2;
        ///            } else {
        xOffset = word.getXOffset();
        yOffset = word.getYOffset();
        ///            }
        ///        }

        // word wrapping
        if (word.isText() && word.getWidth() + MARGIN_LEFT > right) {
            // align and end the current line
            alignWordParts(lineStart, lineHeight, width);
            y += lineHeight;

            int currentPartStart = 0;
            int currentLineWidth = 0;

            // go through the text, break text when it doesn't fit in the line anymore
            for (int i = 1; i <= word.getText().length(); i++) {
                currentLineWidth += word.getCharWidth(i - 1);

                if (currentLineWidth + MARGIN_LEFT > right) {
                    // add the current line
                    QString mid = word.getText().mid(currentPartStart, i - currentPartStart - 1);

                    this->wordParts.push_back(WordPart(word, MARGIN_LEFT, y, currentLineWidth,
                                                       word.getHeight(), lineNumber, mid, mid,
                                                       false, currentPartStart));

                    y += word.getFontMetrics().height();

                    currentPartStart = i - 1;

                    currentLineWidth = 0;
                    lineNumber++;
                }
            }

            QString mid(word.getText().mid(currentPartStart));
            currentLineWidth = word.getFontMetrics().width(mid);

            this->wordParts.push_back(WordPart(word, MARGIN_LEFT, y - word.getHeight(),
                                               currentLineWidth, word.getHeight(), lineNumber, mid,
                                               mid, true, currentPartStart));

            x = currentLineWidth + MARGIN_LEFT + spaceWidth;
            lineHeight = word.getHeight();
            lineStart = this->wordParts.size() - 1;

            first = false;
        }
        // fits in the current line
        else if (first || x + word.getWidth() + xOffset <= right) {
            this->wordParts.push_back(
                WordPart(word, x, y - word.getHeight(), lineNumber, word.getCopyText()));

            x += word.getWidth() + xOffset;
            x += spaceWidth;

            lineHeight = std::max(word.getHeight(), lineHeight);

            first = false;
        }
        // doesn't fit in the line
        else {
            // align and end the current line
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

    // align and end the current line
    alignWordParts(lineStart, lineHeight, width);

    // update height
    int oldHeight = this->height;
    this->height = y + lineHeight + MARGIN_BOTTOM;

    // invalidate buffer if height changed
    if (oldHeight != this->height) {
        this->buffer = nullptr;
    }

    updateBuffer = true;
}

void MessageRef::updateTextSizes()
{
    for (auto &word : this->message->getWords()) {
        if (!word.isText())
            continue;

        QFontMetrics &metrics = word.getFontMetrics();
        word.setSize((int)(metrics.width(word.getText()) * this->dpiMultiplyer),
                     (int)(metrics.height() * this->dpiMultiplyer));
    }
}

void MessageRef::updateImageSizes()
{
    const int mediumTextLineHeight =
        FontManager::getInstance().getFontMetrics(FontManager::Medium).height();
    const qreal emoteScale = SettingsManager::getInstance().emoteScale.get() * this->dpiMultiplyer;
    const bool scaleEmotesByLineHeight = SettingsManager::getInstance().scaleEmotesByLineHeight;

    for (auto &word : this->message->getWords()) {
        if (!word.isImage())
            continue;

        auto &image = word.getImage();

        qreal w = image.getWidth();
        qreal h = image.getHeight();

        if (scaleEmotesByLineHeight) {
            word.setSize(w * mediumTextLineHeight / h * emoteScale,
                         mediumTextLineHeight * emoteScale);
        } else {
            word.setSize(w * image.getScale() * emoteScale, h * image.getScale() * emoteScale);
        }
    }
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

// XXX(pajlada): This is probably not the optimal way to calculate this
int MessageRef::getLastCharacterIndex() const
{
    // find out in which line the cursor is
    int lineNumber = 0, lineStart = 0, lineEnd = 0;

    for (size_t i = 0; i < this->wordParts.size(); i++) {
        const WordPart &part = this->wordParts[i];

        if (part.getLineNumber() != lineNumber) {
            lineStart = i;
            lineNumber = part.getLineNumber();
        }

        lineEnd = i + 1;
    }

    // count up to the cursor
    int index = 0;

    for (int i = 0; i < lineStart; i++) {
        const WordPart &part = this->wordParts[i];

        index += part.getWord().isImage() ? 2 : part.getText().length() + 1;
    }

    for (int i = lineStart; i < lineEnd; i++) {
        const WordPart &part = this->wordParts[i];

        index += part.getCharacterLength();
    }

    return index;
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
