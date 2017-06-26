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
    QString code;
};

class Emojis
{
public:
    static QString replaceShortCodes(const QString &text);

private:
    static QRegularExpression findShortCodesRegex;
};

}  // namespace chatterino
