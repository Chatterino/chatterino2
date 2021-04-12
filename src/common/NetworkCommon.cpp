#include "common/NetworkCommon.hpp"

#include <QStringList>

namespace chatterino {

std::vector<std::pair<QString, QString>> parseHeaderList(
    const QString &headerListString)
{
    std::vector<std::pair<QString, QString>> res;

    // Split the string into a list of header pairs
    // e.g. "Authorization:secretkey;NextHeader:boo" turning into ["Authorization:secretkey","NextHeader:boo"]
    auto headerPairs = headerListString.split(";");

    for (const auto &headerPair : headerPairs)
    {
        // Split the header pair into a list of parts
        // We expect the first part to be the header name, and the second part to be the header value
        auto headerParts = headerPair.trimmed().split(":");

        if (headerParts.size() != 2)
        {
            // The header part either didn't contain a : or contained too many :, making it an invalid header pair
            // Skip the value
            continue;
        }

        const auto headerName = headerParts[0].trimmed().toUtf8();
        const auto headerValue = headerParts[1].trimmed().toUtf8();

        res.emplace_back(headerName, headerValue);
    }

    return res;
}

}  // namespace chatterino
