#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include <functional>

namespace chatterino {

class RatelimitBucket : public QObject
{
public:
    RatelimitBucket(int budget, int cooldown,
                    std::function<void(QString)> callback, QObject *parent);

    void send(QString channel);

private:
    /**
     * @brief budget_ denotes the amount of calls that can be handled before we need to wait for the cooldown
     **/
    int budget_;

    /**
     * @brief This is the amount of time in milliseconds it takes for one used up budget to be put back into the bucket for use elsewhere
     **/
    const int cooldown_;

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
