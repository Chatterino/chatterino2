#pragma once

#include <QList>
#include <QString>

namespace chatterino {

class RatelimitBucket
{
public:
    RatelimitBucket(int limit, int cooldown,
                    std::function<void(QString)> callback);

    void send(QString message);

private:
    int limit_;
    int pending_ = 0;
    int cooldown_;
    std::function<void(QString)> callback_;
    QList<QString> queue_;

    void execute();
};

}  // namespace chatterino
