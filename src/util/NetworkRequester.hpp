#pragma once

#include <QObject>

namespace chatterino {
namespace util {

class NetworkRequester : public QObject
{
    Q_OBJECT

signals:
    void requestUrl();
};

}  // namespace util
}  // namespace chatterino
