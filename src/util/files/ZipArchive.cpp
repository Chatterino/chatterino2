// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/files/ZipArchive.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <QByteArray>
#    include <QString>
#    include <zip.h>

#    include <filesystem>
#    include <span>

using namespace Qt::Literals;

namespace {

using namespace chatterino;

class KubaZip
{
public:
    KubaZip() = default;
    ~KubaZip();

    KubaZip(const KubaZip &) = delete;
    KubaZip &operator=(const KubaZip &) = delete;
    KubaZip(KubaZip &&other) noexcept
    {
        std::swap(this->zip, other.zip);
    }
    KubaZip &operator=(KubaZip &&other) noexcept
    {
        std::swap(this->zip, other.zip);
        return *this;
    }

    operator bool() const noexcept
    {
        return this->zip != nullptr;
    }

    ExpectedStr<void> openMemory(std::span<const char> buf);

    ExpectedStr<size_t> size();

    ExpectedStr<void> findEntry(const char *name);
    ExpectedStr<void> openEntry(size_t n);

    std::string_view entryName();
    ExpectedStr<bool> entryIsSymlink();
    size_t entryUncompressedSize();

    ExpectedStr<QByteArray> extractEntry();

    void destroy();

private:
    void clearEntry();

    struct zip_t *zip = nullptr;
};

auto stringifyError(int err)
{
    return makeUnexpected(QLatin1StringView(zip_strerror(err)));
}

KubaZip::~KubaZip()
{
    this->destroy();
}

ExpectedStr<void> KubaZip::openMemory(std::span<const char> buf)
{
    this->destroy();

    int err = 0;
    this->zip = zip_stream_openwitherror(buf.data(), buf.size(), 0, 'r', &err);
    if (err < 0)
    {
        return stringifyError(err);
    }
    return {};
}

ExpectedStr<size_t> KubaZip::size()
{
    auto sz = zip_entries_total(this->zip);
    if (sz < 0)
    {
        return stringifyError(static_cast<int>(sz));
    }
    return static_cast<size_t>(sz);
}

ExpectedStr<void> KubaZip::findEntry(const char *name)
{
    this->clearEntry();

    int ret = zip_entry_open(this->zip, name);
    if (ret < 0)
    {
        return stringifyError(ret);
    }
    return {};
}

ExpectedStr<void> KubaZip::openEntry(size_t n)
{
    this->clearEntry();

    int ret = zip_entry_openbyindex(this->zip, n);
    if (ret < 0)
    {
        return stringifyError(ret);
    }
    return {};
}

std::string_view KubaZip::entryName()
{
    const auto *ptr = zip_entry_name(this->zip);
    if (!ptr)
    {
        return {};
    }
    return ptr;
}

ExpectedStr<bool> KubaZip::entryIsSymlink()
{
    int ret = zip_entry_issymlink(this->zip);
    if (ret < 0)
    {
        return stringifyError(ret);
    }
    return ret != 0;
}

size_t KubaZip::entryUncompressedSize()
{
    return zip_entry_uncomp_size(this->zip);
}

ExpectedStr<QByteArray> KubaZip::extractEntry()
{
    QByteArray ba;
    int ret = zip_entry_extract(
        this->zip,
        +[](void *arg, uint64_t /*offset*/, const void *data, size_t size) {
            auto *ba = static_cast<QByteArray *>(arg);
            ba->append(static_cast<const char *>(data),
                       static_cast<qsizetype>(size));
            return size;
        },
        &ba);
    if (ret < 0)
    {
        return stringifyError(ret);
    }
    return ba;
}

void KubaZip::destroy()
{
    if (this->zip)
    {
        this->clearEntry();
        zip_close(this->zip);
        this->zip = nullptr;
    }
}

void KubaZip::clearEntry()
{
    if (this->zip)
    {
        zip_entry_close(this->zip);
    }
}

constexpr qsizetype MAX_FILE_SIZE = 8LL * (1 << 20);  // 8 MiB
constexpr qsizetype MAX_ZIP_SIZE = 20LL * (1 << 20);  // 20 MiB

bool isAcceptedEntry(KubaZip &zip)
{
    if (zip.entryIsSymlink().value_or(true))
    {
        return false;
    }

    // Check for weird names that reference parent directories.
    std::string_view nameView = zip.entryName();
    if (nameView.starts_with("..") || nameView.ends_with("/..") ||
        nameView.contains("\\") || nameView.contains("/../") ||
        nameView.starts_with('/') ||                  // Absolute paths
        (nameView.size() >= 2 && nameView[1] == ':')  // C:
    )
    {
        return false;
    }

    return zip.entryUncompressedSize() <= MAX_FILE_SIZE;
}

enum class LoopAction : uint8_t {
    Continue,
    Stop,
};

}  // namespace

namespace chatterino {

class ZipArchivePrivate
{
public:
    ZipArchivePrivate() = default;

    ExpectedStr<void> verifyZip();

    QByteArray backingMemory;
    KubaZip zip;
};

ExpectedStr<void> ZipArchivePrivate::verifyZip()
{
    if (!this->zip)
    {
        return makeUnexpected(u"No zip"_s);
    }

    auto nEntries = this->zip.size();
    if (!nEntries)
    {
        return makeUnexpected(std::move(nEntries).error());
    }
    if (*nEntries == 0)
    {
        return makeUnexpected(u"Empty zip file"_s);
    }

    for (size_t i = 0; i < *nEntries; ++i)
    {
        auto res = this->zip.openEntry(i);
        if (!res)
        {
            return res;
        }
        if (!isAcceptedEntry(this->zip))
        {
            QString msg = u"Zip file contains bad file: '"_s;
            msg.append(this->zip.entryName());
            msg.append('\"');
            return makeUnexpected(std::move(msg));
        }
    }

    return {};
}

ZipArchive::ZipArchive() = default;
ZipArchive::~ZipArchive() = default;

ExpectedStr<void> ZipArchive::openMemory(QByteArray data)
{
    if (this->private_)
    {
        return makeUnexpected(u"Already opened."_s);
    }
    this->private_ = std::make_unique<ZipArchivePrivate>();
    this->private_->backingMemory = std::move(data);

    qsizetype dataSize = this->private_->backingMemory.size();
    if (dataSize >= MAX_ZIP_SIZE)
    {
        return makeUnexpected(u"Buffer too big"_s);
    }

    auto ret = this->private_->zip.openMemory(
        {this->private_->backingMemory.constData(),
         static_cast<size_t>(dataSize)});

    return this->private_->verifyZip();
}

ExpectedStr<QByteArray> ZipArchive::readEntry(const char *name)
{
    if (!this->private_)
    {
        return makeUnexpected(u"No file"_s);
    }

    auto ret = this->private_->zip.findEntry(name);
    if (!ret)
    {
        return makeUnexpected(std::move(ret).error());
    }
    return this->private_->zip.extractEntry();
}

ExpectedStr<void> ZipArchive::extractTo(const std::filesystem::path &directory)
{
    if (!this->private_ || this->private_->backingMemory.isEmpty())
    {
        return makeUnexpected(u"No file"_s);
    }

    QByteArrayView buf(this->private_->backingMemory);
    int ret =
        zip_stream_extract(buf.constData(), static_cast<size_t>(buf.size()),
                           directory.string().c_str(), nullptr, nullptr);
    if (ret < 0)
    {
        return stringifyError(ret);
    }
    return {};
}

}  // namespace chatterino

#endif
