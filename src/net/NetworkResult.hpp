#pragma once

#include <QJsonObject>

namespace chatterino
{
    class NetworkResult
    {
    public:
        NetworkResult(const QByteArray& data);

        QJsonObject parseJson() const;
        const QByteArray& getData() const;

    private:
        QByteArray data_;
    };

}  // namespace chatterino
