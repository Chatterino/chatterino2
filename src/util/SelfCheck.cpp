#include "util/SelfCheck.hpp"

#include "common/Literals.hpp"

#include <QBuffer>
#include <QImageReader>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QStringBuilder>
#include <QStringList>

namespace {

using namespace chatterino::literals;

// lossy image from https://developers.google.com/speed/webp/faq#in_your_own_javascript
const QByteArray WEBP_IMAGE =
    "RIFF\x22\0\0\0WEBPVP8\x20\x16\0\0\0\x30\x01\0\x9d\x01\x2a\x01\0\x01\0\x0e\xc0\xfe\x25\xa4\0\x03p\0\0\0\0"_ba;

}  // namespace

namespace chatterino::selfcheck {

void checkWebp()
{
    QStringList messages;
    QMimeDatabase mimeDb;

    auto mime = mimeDb.mimeTypeForData(WEBP_IMAGE);
    if (!mime.inherits("image/webp"))
    {
        messages.emplace_back(u"Failed to determine MIME type for WEBP image - "
                              u"detected MIME type: \"" %
                              mime.name() % '"');
    }

    QBuffer buffer;
    buffer.setData(WEBP_IMAGE);
    QImageReader reader(&buffer);

    if (reader.canRead())
    {
        if (reader.imageCount() != 1)
        {
            messages.emplace_back(u"Minimal WEBP image doesn't have one (1) "
                                  u"expected image - got: " %
                                  QString::number(reader.imageCount()) %
                                  u" - error: " % reader.errorString());
        }
        reader.read();
    }
    else
    {
        messages.emplace_back(
            u"Minimal WEBP image can't be read - QImageReader::canRead "
            u"returned false - error: \"" %
            reader.errorString() % '"');
    }

    if (!messages.empty())
    {
        QMessageBox::warning(
            nullptr, u"Chatterino - Sanity Check"_s,
            u"Your Chatterino instance is not able to load WEBP files.\nMake "
            u"sure you have qtimageformats installed.\n\n" %
                messages.join(u"\n\n"));
    }
}

}  // namespace chatterino::selfcheck
