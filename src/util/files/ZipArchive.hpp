// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "util/Expected.hpp"

#    include <QByteArray>

#    include <filesystem>
#    include <memory>

namespace chatterino {

class ZipArchivePrivate;
class ZipArchive
{
public:
    ZipArchive();
    ~ZipArchive();

    ExpectedStr<void> openMemory(QByteArray data);

    ExpectedStr<QByteArray> readEntry(const char *name);

    ExpectedStr<void> extractTo(const std::filesystem::path &directory);

private:
    std::unique_ptr<ZipArchivePrivate> private_;
};

}  // namespace chatterino

#endif
