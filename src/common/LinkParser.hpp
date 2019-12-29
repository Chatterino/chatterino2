#pragma once

#include <QRegularExpressionMatch>
#include <QString>

namespace chatterino {

class LinkParser
{
public:
    explicit LinkParser(const QString &unparsedString);

    bool hasMatch() const;
    QString getCaptured() const;

private:
    bool hasMatch_;
    QString match_;
};

}  // namespace chatterino
