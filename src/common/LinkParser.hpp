#pragma once

#include <QString>

#include <optional>

namespace chatterino {

struct ParsedLink {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    using StringView = QStringView;
#else
    using StringView = QStringRef;
#endif
    StringView protocol;
    StringView host;
    StringView rest;
    QString source;
};

class LinkParser
{
public:
    explicit LinkParser(const QString &unparsedString);

    const std::optional<ParsedLink> &result() const;

private:
    std::optional<ParsedLink> result_{};
};

}  // namespace chatterino
