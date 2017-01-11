#include <QStringBuilder>
#include <QFile>
#include <QTextStream>

#include "emojis.h"
#include "emotes.h"

QRegularExpression* Emojis::findShortCodesRegex = new QRegularExpression(":([-+\\w]+):");

QMap<QString, Emojis::EmojiData>* Emojis::shortCodeToEmoji = new QMap<QString, Emojis::EmojiData>();
QMap<QString, QString>* Emojis::emojiToShortCode = new QMap<QString, QString>();
QMap<QChar, QMap<QString, QString>>* Emojis::firstEmojiChars = new QMap<QChar, QMap<QString, QString>>();

ConcurrentMap<QString, LazyLoadedImage*>* Emojis::imageCache = new ConcurrentMap<QString, LazyLoadedImage*>();

QString Emojis::replaceShortCodes(const QString &text)
{
#pragma message WARN("xD")
    return text;
}

void Emojis::parseEmojis(std::vector<std::tuple<LazyLoadedImage*, QString>>& vector, const QString& text)
{
    long lastSlice = 0;

    for (auto i = 0; i < text.length() - 1; i++)
    {
        if (!text.at(i).isLowSurrogate())
        {
            auto iter = firstEmojiChars->find(text.at(i));

            if (iter != firstEmojiChars->end())
            {
                for (auto j = std::min(8, text.length() - i); j > 0; j--)
                {
                    QString emojiString = text.mid(i, 2);
                    auto emojiIter = iter.value().find(emojiString);

                    if (emojiIter != iter.value().end())
                    {
                        QString url = "https://cdnjs.cloudflare.com/ajax/libs/emojione/2.2.6/assets/png/" + emojiIter.value() + ".png";

                        if (i - lastSlice != 0)
                        {
                            vector.push_back(std::tuple<LazyLoadedImage*, QString>(NULL, text.mid(lastSlice, i - lastSlice)));
                        }

                        vector.push_back(std::tuple<LazyLoadedImage*, QString>(imageCache->getOrAdd(url, [&url]{ return new LazyLoadedImage(url, 0.35); }), QString()));

                        i += j - 1;

                        lastSlice = i + 1;

                        break;
                    }
                }
            }
        }
    }

    if (lastSlice < text.length())
    {
        vector.push_back(std::tuple<LazyLoadedImage*, QString>(NULL, text.mid(lastSlice)));
    }
}

void Emojis::initEmojis()
{
    QFile file(":/emojidata.txt");
    file.open(QFile::ReadOnly);
    QTextStream in(&file);

    uint emotes[4];

    while (!in.atEnd())
    {
        QString line = in.readLine();

        if (line.length() < 3 || line.at(0) == '#') continue;

        QStringList a = line.split(' ');
        if (a.length() < 2) continue;

        QStringList b = a.at(1).split('-');
        if (b.length() < 1) continue;

        int i = 0;

        for (const QString& item : b)
        {
            emotes[i++] = QString(item).toUInt(nullptr, 16);
        }

        shortCodeToEmoji->insert(a.at(0), Emojis::EmojiData { QString::fromUcs4(emotes, i), a.at(1) });
    }

    for (auto const& emoji : shortCodeToEmoji->toStdMap())
    {
        emojiToShortCode->insert(emoji.second.value, emoji.first);
    }

    for (auto const& emoji : shortCodeToEmoji->toStdMap())
    {
        auto iter = firstEmojiChars->find(emoji.first.at(0));

        if (iter != firstEmojiChars->end())
        {
            iter.value().insert(emoji.second.value, emoji.second.value);
            continue;
        }

        firstEmojiChars->insert(emoji.first.at(0), QMap<QString, QString>{{ emoji.second.value, emoji.second.code }});
    }
}
