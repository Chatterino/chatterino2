#pragma once

#include <QString>
#include <tuple>

namespace chatterino
{
    inline std::pair<QStringRef, QStringRef> splitPairRef(
        const QStringRef& str, QChar c)
    {
        static QString empty;
        static QStringRef ref(&empty);

        auto index = str.indexOf(c);

        if (index == -1)
            return {ref, ref};
        else
            return {str.mid(0, index), str.mid(index + 1)};
    }

    inline std::pair<QString, QString> splitPair(const QStringRef& str, QChar c)
    {
        static QString empty;

        auto index = str.indexOf(c);

        if (index == -1)
            return {empty, empty};
        else
            return {
                str.mid(0, index).toString(), str.mid(index + 1).toString()};
    }
}  // namespace chatterino
