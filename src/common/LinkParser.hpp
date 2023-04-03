#pragma once

#include <QString>

#include <optional>

namespace chatterino {

struct ParsedLink {
    QStringView protocol;
    QStringView host;
    QStringView rest;
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
