#include "messageref.h"
#include "emotes.h"
#include "settings.h"

#define MARGIN_LEFT 8
#define MARGIN_RIGHT 8
#define MARGIN_TOP 8
#define MARGIN_BOTTOM 8

namespace chatterino {
namespace messages {

MessageRef::MessageRef(std::shared_ptr<Message> message)
    : message(message.get())
    , messagePtr(message)
    , wordParts()
{
}

bool
MessageRef::layout(int width, bool enableEmoteMargins)
{
    auto &settings = Settings::getInstance();

    width = width - (width % 2);

    int mediumTextLineHeight = Fonts::getFontMetrics(Fonts::Medium).height();
    int spaceWidth = 4;

    bool redraw = width != this->currentLayoutWidth || this->relayoutRequested;

    bool recalculateImages = this->emoteGeneration != Emotes::getGeneration();
    bool recalculateText = this->fontGeneration != Fonts::getGeneration();

    qreal emoteScale = settings.emoteScale.get();
    bool scaleEmotesByLineHeight = settings.scaleEmotesByLineHeight.get();

    if (recalculateImages || recalculateText) {
        this->emoteGeneration = Emotes::getGeneration();
        this->fontGeneration = Fonts::getGeneration();

        redraw = true;

        for (auto &word : this->message->getWords()) {
            if (word.isImage()) {
                if (recalculateImages) {
                    auto &image = word.getImage();

                    qreal w = image.getWidth();
                    qreal h = image.getHeight();

                    if (scaleEmotesByLineHeight) {
                        word.setSize(w * mediumTextLineHeight / h * emoteScale,
                                     mediumTextLineHeight * emoteScale);
                    } else {
                        word.setSize(w * image.getScale() * emoteScale,
                                     h * image.getScale() * emoteScale);
                    }
                }
            } else {
                if (recalculateText) {
                    QFontMetrics &metrics = word.getFontMetrics();
                    word.setSize(metrics.width(word.getText()),
                                 metrics.height());
                }
            }
        }
    }

    if (!redraw) {
        return false;
    }

    int x = MARGIN_LEFT;
    int y = MARGIN_TOP;

    int right = width - MARGIN_RIGHT - MARGIN_LEFT;

    int lineStart = 0;
    int lineHeight = 0;
    bool first = true;

    this->wordParts.clear();

    uint32_t flags = Settings::getInstance().getWordTypeMask();

    for (auto it = this->message->getWords().begin();
         it != this->message->getWords().end(); ++it) {
        Word &word = *it;

        if ((word.getType() & flags) == Word::None) {
            continue;
        }

        int xOffset = 0, yOffset = 0;

        if (enableEmoteMargins) {
            if (word.isImage() && word.getImage().getIsHat()) {
                xOffset = -word.getWidth() + 2;
            } else {
                xOffset = word.getXOffset();
                yOffset = word.getYOffset();
            }
        }

        // word wrapping
        if (word.isText() && word.getWidth() + MARGIN_LEFT > right) {
            this->alignWordParts(lineStart, lineHeight);

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

                    this->wordParts.push_back(WordPart(word, MARGIN_LEFT, y,
                                                       width, word.getHeight(),
                                                       mid, mid));

                    y += metrics.height();

                    start = i - 1;

                    width = 0;
                }
            }

            QString mid(text.mid(start));
            width = metrics.width(mid);

            this->wordParts.push_back(WordPart(word, MARGIN_LEFT,
                                               y - word.getHeight(), width,
                                               word.getHeight(), mid, mid));
            x = width + MARGIN_LEFT + spaceWidth;

            lineHeight = word.getHeight();

            lineStart = this->wordParts.size() - 1;

            first = false;
        } else if (first || x + word.getWidth() + xOffset <= right) {
            // fits in the line
            this->wordParts.push_back(
                WordPart(word, x, y - word.getHeight(), word.getCopyText()));

            x += word.getWidth() + xOffset;
            x += spaceWidth;

            lineHeight = std::max(word.getHeight(), lineHeight);

            first = false;
        } else {
            // doesn't fit in the line
            this->alignWordParts(lineStart, lineHeight);

            y += lineHeight;

            this->wordParts.push_back(WordPart(
                word, MARGIN_LEFT, y - word.getHeight(), word.getCopyText()));

            lineStart = this->wordParts.size() - 1;

            lineHeight = word.getHeight();

            x = word.getWidth() + MARGIN_LEFT;
            x += spaceWidth;
        }
    }

    this->alignWordParts(lineStart, lineHeight);

    this->height = y + lineHeight;

    return true;
}

void
MessageRef::alignWordParts(int lineStart, int lineHeight)
{
    for (size_t i = lineStart; i < this->wordParts.size(); i++) {
        WordPart &wordPart2 = this->wordParts.at(i);

        wordPart2.setY(wordPart2.getY() + lineHeight);
    }
}
}
}
