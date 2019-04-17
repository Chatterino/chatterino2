#pragma once

#include <QString>
#include <tuple>

namespace chatterino
{
    std::pair<QStringRef, QStringRef> splitPairRef(
        const QStringRef& str, QChar c);
    std::pair<QString, QString> splitPair(const QStringRef& str, QChar c);
    int unicodeLength(const QString& str);
}  // namespace chatterino
