#pragma once

#include <QObject>

namespace chatterino
{
    class NetworkRequester : public QObject
    {
        Q_OBJECT

    signals:
        void requestUrl();
    };

}  // namespace chatterino
