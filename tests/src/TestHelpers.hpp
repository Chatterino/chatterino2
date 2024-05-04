#pragma once

#include <ostream>

class QString;
class QStringView;
class QByteArray;

std::ostream &operator<<(std::ostream &os, QStringView str);
std::ostream &operator<<(std::ostream &os, const QByteArray &bytes);
std::ostream &operator<<(std::ostream &os, const QString &str);

// NOLINTBEGIN(readability-identifier-naming)
void PrintTo(const QByteArray &bytes, std::ostream *os);
void PrintTo(QStringView str, std::ostream *os);
void PrintTo(const QString &str, std::ostream *os);
// NOLINTEND(readability-identifier-naming)
