#pragma once

#include <QString>

namespace chatterino {

class LinkParser
{
public:
    explicit LinkParser(const QString &unparsedString);

    bool hasMatch() const;
    QString getCaptured() const;

private:
    bool hasMatch_{false};
    QString match_;
};

}  // namespace chatterino
