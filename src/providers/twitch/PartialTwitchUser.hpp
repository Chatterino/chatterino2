#pragma once

#include <QObject>
#include <QString>

#include <functional>

namespace chatterino
{
    // Experimental class to test a method of calling APIs on twitch users
    class PartialTwitchUser
    {
        PartialTwitchUser() = default;

        QString username_;
        QString id_;

    public:
        static PartialTwitchUser byName(const QString& username);
        static PartialTwitchUser byId(const QString& id);

        void getId(std::function<void(QString)> successCallback,
            const QObject* caller = nullptr);
    };

}  // namespace chatterino
