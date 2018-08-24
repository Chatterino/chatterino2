#pragma once

#include <QString>
#include <functional>

namespace chatterino {

class BttvTooltip
{
public:
    static void getUrlTooltip(const QString url,
                           std::function<void(QString)> callback);

private:
};

}  // namespace chatterino
