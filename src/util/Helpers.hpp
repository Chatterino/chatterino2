#pragma once

#include <QString>

namespace chatterino {

QString generateUuid();

QString formatRichLink(const QString &url, bool file = false);

QString formatRichNamedLink(const QString &url, const QString &name,
                            bool file = false);

QString shortenString(const QString &str, unsigned maxWidth = 50);

QString localizeNumbers(const int &number);

QString kFormatNumbers(const int &number);

}  // namespace chatterino
