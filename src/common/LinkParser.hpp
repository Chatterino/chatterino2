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
    QRegularExpressionMatch match_;
};

}  // namespace chatterino
