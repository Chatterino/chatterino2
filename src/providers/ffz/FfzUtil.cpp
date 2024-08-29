#include "providers/ffz/FfzUtil.hpp"

#include <QUrl>

namespace chatterino {

Url parseFfzUrl(const QString &ffzUrl)
{
    QUrl asURL(ffzUrl);
    asURL.setScheme("https");
    return {asURL.toString()};
}

}  // namespace chatterino
