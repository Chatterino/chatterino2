#include "common/network/NetworkCommon.hpp"

#include <QStringList>

namespace chatterino {

std::vector<std::pair<QByteArray, QByteArray>> parseHeaderList(
    const QString &headerListString)
{
    std::vector<std::pair<QByteArray, QByteArray>> res;

    // Split the string into a list of header pairs
    // e.g. "Authorization:secretkey;NextHeader:boo" turning into ["Authorization:secretkey","NextHeader:boo"]
    auto headerPairs = headerListString.split(";");

    for (const auto &headerPair : headerPairs)
    {
        const auto headerName =
            headerPair.section(":", 0, 0).trimmed().toUtf8();
        const auto headerValue = headerPair.section(":", 1).trimmed().toUtf8();

        if (headerName.isEmpty() || headerValue.isEmpty())
        {
            // The header part either didn't contain a : or the name/value was empty
            // Skip the value
            continue;
        }

        res.emplace_back(headerName, headerValue);
    }

    return res;
}

}  // namespace chatterino
