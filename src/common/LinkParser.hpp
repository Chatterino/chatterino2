#pragma once

#include <QRegularExpressionMatch>
#include <QString>

namespace chatterino {

class LinkParser
{
public:
    explicit LinkParser(const QString &unparsedString);

    bool hasMatch() const
    {
        return this->match_.hasMatch();
    }

    QString getCaptured() const
    {
        return this->match_.captured();
    }

private:
    QRegularExpressionMatch match_;
};

}  // namespace chatterino
