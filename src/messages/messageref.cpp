#include "messageref.hpp"
#include "emotemanager.hpp"
#include "settingsmanager.hpp"

#include <QDebug>

#define MARGIN_LEFT 8
#define MARGIN_RIGHT 8
#define MARGIN_TOP 8
#define MARGIN_BOTTOM 8

using namespace chatterino::messages;

namespace chatterino {
namespace messages {

MessageRef::MessageRef(SharedMessage message)
    : _message(message)
    , _wordParts()
{
}

Message *MessageRef::getMessage()
{
    return _message.get();
}

int MessageRef::getHeight() const
{
    return _height;
}

bool MessageRef::layout(int width, bool enableEmoteMargins)
{
    auto &settings = SettingsManager::getInstance();

    bool sizeChanged = width != _currentLayoutWidth;
    bool redraw = width != _currentLayoutWidth;
    int spaceWidth = 4;

    int mediumTextLineHeight =
        FontManager::getInstance().getFontMetrics(FontManager::Medium).height();

    bool recalculateImages = _emoteGeneration != EmoteManager::getInstance().getGeneration();
    bool recalculateText = _fontGeneration != FontManager::getInstance().getGeneration();
    bool newWordTypes = _currentWordTypes != SettingsManager::getInstance().getWordTypeMask();

    qreal emoteScale = settings.emoteScale.get();
    bool scaleEmotesByLineHeight = settings.scaleEmotesByLineHeight.get();

    // calculate word sizes
    if (!redraw && !recalculateImages && !recalculateText && !newWordTypes) {
        return false;
    }

    _emoteGeneration = EmoteManager::getInstance().getGeneration();
    _fontGeneration = FontManager::getInstance().getGeneration();

    for (auto &word : _message->getWords()) {
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
        _currentWordTypes = SettingsManager::getInstance().getWordTypeMask();
    }

    // layout
    _currentLayoutWidth = width;

    int x = MARGIN_LEFT;
    int y = MARGIN_TOP;

    int right = width - MARGIN_RIGHT;

    int lineNumber = 0;
    int lineStart = 0;
    int lineHeight = 0;
    bool first = true;

    _wordParts.clear();

    uint32_t flags = SettingsManager::getInstance().getWordTypeMask();

    for (auto it = _message->getWords().begin(); it != _message->getWords().end(); ++it) {
        Word &word = *it;

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
            alignWordParts(lineStart, lineHeight);

            y += lineHeight;

            const QString &text = word.getText();

            int start = 0;
            QFontMetrics &metrics = word.getFontMetrics();

            int width = 0;

            std::vector<short> &charWidths = word.getCharacterWidthCache();

            if (charWidths.size() == 0) {
                for (int i = 0; i < text.length(); i++) {
                    charWidths.push_back(metrics.charWidth(text, i));
                }
            }

            for (int i = 2; i <= text.length(); i++) {
                if ((width = width + charWidths[i - 1]) + MARGIN_LEFT > right) {
                    QString mid = text.mid(start, i - start - 1);

                    _wordParts.push_back(WordPart(word, MARGIN_LEFT, y, width, word.getHeight(),
                                                  lineNumber, mid, mid));

                    y += metrics.height();

                    start = i - 1;

                    width = 0;
                    lineNumber++;
                }
            }

            QString mid(text.mid(start));
            width = metrics.width(mid);

            _wordParts.push_back(WordPart(word, MARGIN_LEFT, y - word.getHeight(), width,
                                          word.getHeight(), lineNumber, mid, mid));
            x = width + MARGIN_LEFT + spaceWidth;

            lineHeight = word.getHeight();

            lineStart = _wordParts.size() - 1;

            first = false;
        } else if (first || x + word.getWidth() + xOffset <= right) {
            // fits in the line
            _wordParts.push_back(
                WordPart(word, x, y - word.getHeight(), lineNumber, word.getCopyText()));

            x += word.getWidth() + xOffset;
            x += spaceWidth;

            lineHeight = std::max(word.getHeight(), lineHeight);

            first = false;
        } else {
            // doesn't fit in the line
            alignWordParts(lineStart, lineHeight);

            y += lineHeight;

            _wordParts.push_back(
                WordPart(word, MARGIN_LEFT, y - word.getHeight(), lineNumber, word.getCopyText()));

            lineStart = _wordParts.size() - 1;

            lineHeight = word.getHeight();

            x = word.getWidth() + MARGIN_LEFT;
            x += spaceWidth;

            lineNumber++;
        }
    }

    alignWordParts(lineStart, lineHeight);

    if (_height != y + lineHeight) {
        sizeChanged = true;
        _height = y + lineHeight;
    }

    if (sizeChanged) {
        buffer = nullptr;
    }

    updateBuffer = true;

    return true;
}

const std::vector<WordPart> &MessageRef::getWordParts() const
{
    return _wordParts;
}

void MessageRef::alignWordParts(int lineStart, int lineHeight)
{
    for (size_t i = lineStart; i < _wordParts.size(); i++) {
        WordPart &wordPart2 = _wordParts.at(i);

        wordPart2.setY(wordPart2.getY() + lineHeight);
    }
}

bool MessageRef::tryGetWordPart(QPoint point, Word &word)
{
    // go through all words and return the first one that contains the point.
    for (WordPart &wordPart : _wordParts) {
        if (wordPart.getRect().contains(point)) {
            word = wordPart.getWord();
            return true;
        }
    }

    return false;
}

int MessageRef::getSelectionIndex(QPoint position)
{
    if (_wordParts.size() == 0) {
        return 0;
    }

    // find out in which line the cursor is
    int lineNumber = 0, lineStart = 0, lineEnd = 0;

    for (int i = 0; i < _wordParts.size(); i++) {
        WordPart &part = _wordParts[i];

        if (part.getLineNumber() != 0 && position.y() < part.getY()) {
            break;
        }

        if (part.getLineNumber() != lineNumber) {
            lineStart = i - 1;
            lineNumber = part.getLineNumber();
        }

        lineEnd = part.getLineNumber() == 0 ? i : i + 1;
    }

    // count up to the cursor
    int index = 0;

    for (int i = 0; i < lineStart; i++) {
        WordPart &part = _wordParts[i];

        index += part.getWord().isImage() ? 2 : part.getText().length() + 1;
    }

    for (int i = lineStart; i < lineEnd; i++) {
        WordPart &part = _wordParts[i];

        // curser is left of the word part
        if (position.x() < part.getX()) {
            break;
        }

        // cursor is right of the word part
        if (position.x() > part.getX() + part.getWidth()) {
            index += part.getWord().isImage() ? 2 : part.getText().length() + 1;
            continue;
        }

        // cursor is over the word part
        if (part.getWord().isImage()) {
            index++;
        } else {
            auto text = part.getWord().getText();

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
