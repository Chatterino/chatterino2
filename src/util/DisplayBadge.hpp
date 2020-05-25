#pragma once

namespace chatterino {
class DisplayBadge
{
public:
    DisplayBadge(QString displayName, QString badgeName, QString badgeVersion);

    QString displayName() const;
    QString badgeName() const;
    QString badgeVersion() const;
    QString identifier() const;

private:
    QString displayName_;
    QString badgeName_;
    QString badgeVersion_;
};

}  // namespace chatterino
