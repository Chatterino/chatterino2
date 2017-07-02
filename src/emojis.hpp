#pragma once

#include "concurrentmap.hpp"
#include "messages/lazyloadedimage.hpp"

#include <QObject>
#include <QRegularExpression>
#include <QString>

#include <unordered_map>

namespace chatterino {

struct EmojiData {
    QString value;

    // what's used in the emoji-one url
    QString code;

    // i.e. thinking
    QString shortCode;
};

class Emojis
{
public:
    static QString replaceShortCodes(const QString &text);

private:
    static QRegularExpression findShortCodesRegex;
};

}  // namespace chatterino
