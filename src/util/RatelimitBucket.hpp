#pragma once

#include <QList>
#include <QObject>
#include <QString>

namespace chatterino {

class RatelimitBucket : public QObject
{
public:
    RatelimitBucket(int limit, int cooldown,
                    std::function<void(QString)> callback, QObject *parent);

    void send(QString message);

private:
    int limit_;
    int pending_ = 0;
    int cooldown_;
    std::function<void(QString)> callback_;
    QList<QString> queue_;

    /**
     * @brief Run the callback on one entry in the queue.
     *
     * This will start a timer that runs after cooldown_ milliseconds that
     * gives back one "token" to the bucket and calls handleOne again.
     **/
    void handleOne();
};

}  // namespace chatterino
