#pragma once

#include <QString>

namespace chatterino {

class DisplayBadge
{
public:
    DisplayBadge(QString displayName, QString badgeName);

    QString displayName() const;
    QString badgeName() const;

private:
    QString displayName_;
    QString badgeName_;
};

}  // namespace chatterino
