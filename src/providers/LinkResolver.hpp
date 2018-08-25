#pragma once

#include <QString>
#include <functional>

namespace chatterino {

class LinkResolver
{
public:
    static void getLinkInfo(const QString url,
                           std::function<void(QString)> callback);

private:
};

}  // namespace chatterino
