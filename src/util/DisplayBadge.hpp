#pragma once

#include <QString>

namespace chatterino {

class DisplayBadge
{
public:
    DisplayBadge(QString displayName, QString badgeName);

    QString displayName() const;
    QString badgeName() const;

    bool operator<(const DisplayBadge& other) const
    {
        return displayName() < other.displayName();
    }

private:
    QString displayName_;
    QString badgeName_;
};

}  // namespace chatterino
