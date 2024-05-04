#include "TestHelpers.hpp"

#include <QString>
#include <QStringView>

std::ostream &operator<<(std::ostream &os, QStringView str)
{
    os << str.toString();
    return os;
}

std::ostream &operator<<(std::ostream &os, const QByteArray &bytes)
{
    os << bytes.toStdString();
    return os;
}

std::ostream &operator<<(std::ostream &os, const QString &str)
{
    os << str.toStdString();
    return os;
}

void PrintTo(const QByteArray &bytes, std::ostream *os)
{
    *os << bytes.toStdString();
}

void PrintTo(QStringView str, std::ostream *os)
{
    *os << str;
}

void PrintTo(const QString &str, std::ostream *os)
{
    *os << str;
}
