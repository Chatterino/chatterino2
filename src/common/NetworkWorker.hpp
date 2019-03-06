#pragma once

#include <QObject>

namespace chatterino
{
    class NetworkWorker : public QObject
    {
        Q_OBJECT

    signals:
        void doneUrl();
    };

}  // namespace chatterino
