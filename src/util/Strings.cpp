#include <util/Strings.hpp>

namespace chatterino
{
    std::pair<QStringRef, QStringRef> splitPairRef(
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

    std::pair<QString, QString> splitPair(const QStringRef& str, QChar c)
    {
        static QString empty;

        auto index = str.indexOf(c);

        if (index == -1)
            return {empty, empty};
        else
            return {
                str.mid(0, index).toString(), str.mid(index + 1).toString()};
    }

    int unicodeLength(const QString& str)
    {
        return std::accumulate(str.begin(), str.end(), 0,
            [](int a, QChar c) { return a + (c.isHighSurrogate() ? 0 : 1); });
    }
}  // namespace chatterino
