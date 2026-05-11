// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "Test.hpp"

#include <QString>
#include <QStringView>

const QString PUBSUB_WSS_ADDR =
    qEnvironmentVariable("CHATTERINO_TEST_PUBSUB_WSS_ADDR", "127.0.0.1:9050");
const QString PUBSUB_WS_ADDR =
    qEnvironmentVariable("CHATTERINO_TEST_PUBSUB_WS_ADDR", "127.0.0.1:9052");

#ifdef CHATTERINO_TEST_USE_PUBLIC_HTTPBIN
// Using our self-hosted version of httpbox https://github.com/kevinastone/httpbox
const QString HTTPBIN_BASE_URL =
    "https://" + qEnvironmentVariable("CHATTERINO_TEST_HTTPBOX_ADDR",
                                      "braize.pajlada.com/httpbox");
#else
const QString HTTPBIN_BASE_URL =
    "http://" +
    qEnvironmentVariable("CHATTERINO_TEST_HTTPBOX_ADDR", "127.0.0.1:9051");
#endif

std::ostream &operator<<(std::ostream &os, QStringView str)
{
    os << str.toString().toStdString();
    return os;
}

std::ostream &operator<<(std::ostream &os, const QByteArray &bytes)
{
    os << std::string_view{bytes.data(), static_cast<size_t>(bytes.size())};
    return os;
}

std::ostream &operator<<(std::ostream &os, const QString &str)
{
    os << str.toStdString();
    return os;
}

// The PrintTo overloads use UniversalPrint to print strings in quotes.
// Even though this uses testing::internal, this is publically documented in
// gtest/gtest-printers.h.

void PrintTo(const QByteArray &bytes, std::ostream *os)
{
    ::testing::internal::UniversalPrint(bytes.toStdString(), os);
}

void PrintTo(QStringView str, std::ostream *os)
{
    ::testing::internal::UniversalPrint(
        std::u16string{str.utf16(), static_cast<size_t>(str.size())}, os);
}

void PrintTo(const QString &str, std::ostream *os)
{
    ::testing::internal::UniversalPrint(str.toStdU16String(), os);
}

std::unique_lock<std::mutex> environmentLock()
{
    static std::mutex m;

    return std::unique_lock(m);
}
