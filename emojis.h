#ifndef EMOJIS_H
#define EMOJIS_H

#include <QRegularExpression>
#include <QObject>
#include <QString>
#include "lazyloadedimage.h"
#include "concurrentmap.h"

class Emojis
{
public:
    std::vector<std::tuple<LazyLoadedImage*, QString>> parseEmotes(const QString& value);

    static void initEmojis();

private:
    static QRegularExpression* findShortCodesRegex;

    static QMap<QString, QString>* shortCodeToEmoji;
    static QMap<QString, QString>* emojiToShortCode;
    static QMap<QChar, QMap<QString, LazyLoadedImage*>>* firstEmojiChars;

    Emojis() {}
};

#endif // EMOJIS_H
