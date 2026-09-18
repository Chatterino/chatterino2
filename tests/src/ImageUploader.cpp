// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/ImageUploader.hpp"

#include "common/network/NetworkResult.hpp"
#include "lib/Snapshot.hpp"
#include "mocks/BaseApplication.hpp"
#include "Test.hpp"
#include "util/ImageUploader.hpp"

#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMimeData>
#include <QString>
#include <QTemporaryDir>
#include <QtEndian>
#include <QUrl>

#include <utility>
#include <vector>

namespace {

constexpr bool UPDATE_SNAPSHOTS = false;

// Two 2x2 APNG frames
const auto ANIMATED_PNG_HEX =
    "89504E470D0A1A0A0000000D4948445200000002000000020802000000FDD49A73"
    "00000009704859730000000100000001004F25C4D6000000086163544C00000002"
    "00000000F38D93700000001A6663544C00000000000000020000000200000000"
    "00000000000100020000E6478DB80000001049444154789C63F8CBC000440C"
    "100A001FAE03F5F6182A590000001A6663544C000000010000000100000001"
    "0000000000000000000100020000CF1F8BBC000000106664415400000002789C"
    "63F8CBC0000002FB00FE6E73CC340000000049454E44AE426082";

// Two 2x2 WebP frames
const auto ANIMATED_WEBP_HEX =
    "52494646C000000057454250565038580A00000002000000010000010000"
    "414E494D06000000FFFFFFFF0000414E4D464800000000000000000001"
    "0000010000640000025650382030000000D001009D012A020002000200"
    "3425A00274BA01F80003B000FEF0C40BFF20B96175C8D7FF203FE407"
    "FC80FFF8F2000000414E4D464400000000000000000001000001000064"
    "000000565038202C0000009401009D012A0200020000003425A00274BA"
    "00039800FEF9936FFF901FFF901FFF901FFF203FE2177B203000";
const auto STATIC_WEBP_HEX =
    "524946463C000000574542505650382030000000D001009D012A020002"
    "0002003425A00274BA01F80003B000FEF0C40BFF20B96175C8D7FF"
    "203FE407FC80FFF8F2000000";

// EXIF with the image description set to "forsen"
const auto PNG_EXIF_CHUNK_HEX =
    "00000021655849664D4D002A000000080001010E0002000000070000001A000000"
    "00666F7273656E00F4C0B558";
const auto WEBP_EXIF_CHUNK_HEX =
    "45584946270000004578696600004D4D002A000000080001010E000200000007"
    "0000001A00000000666F7273656E0000";
const auto JPEG_EXIF_SEGMENT_HEX =
    "FFE100294578696600004D4D002A000000080001010E0002000000070000001A"
    "00000000666F7273656E00";

QByteArray makeImage(const char *format)
{
    QImage image{1, 1, QImage::Format_RGB32};
    image.fill(Qt::red);

    QByteArray data;
    QBuffer buffer{&data};
    EXPECT_TRUE(buffer.open(QIODevice::WriteOnly));
    EXPECT_TRUE(image.save(&buffer, format));
    return data;
}

QByteArray addExifToPng(QByteArray data)
{
    data.insert(data.size() - 12, QByteArray::fromHex(PNG_EXIF_CHUNK_HEX));
    return data;
}

QByteArray addExifToWebp(QByteArray data)
{
    if (QByteArrayView{data}.sliced(12, 4) != "VP8X")
    {
        data.insert(
            12, QByteArray::fromHex("565038580A00000000000000010000010000"));
    }
    data[20] = static_cast<char>(static_cast<uchar>(data[20]) | 0x08);
    data.append(QByteArray::fromHex(WEBP_EXIF_CHUNK_HEX));
    qToLittleEndian(static_cast<quint32>(data.size() - 8), data.data() + 4);
    return data;
}

QByteArray addExifToJpeg(QByteArray data)
{
    data.insert(2, QByteArray::fromHex(JPEG_EXIF_SEGMENT_HEX));
    return data;
}

class ImageUploaderInputTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(this->directory_.isValid());
    }

    auto imagesFromFile(const QString &name, const QByteArray &data)
    {
        const auto path = this->directory_.filePath(name);
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        EXPECT_EQ(file.write(data), data.size());
        file.close();

        QMimeData mimeData;
        mimeData.setUrls({QUrl::fromLocalFile(path)});
        return this->uploader_.getImages(&mimeData);
    }

    auto imagesFromClipboard(const char *mime, const QByteArray &data)
    {
        QMimeData mimeData;
        mimeData.setData(mime, data);
        return this->uploader_.getImages(&mimeData);
    }

    QTemporaryDir directory_;
    chatterino::ImageUploader uploader_;
};

}  // namespace

TEST_F(ImageUploaderInputTest, StripsExifFromAnimatedPngFile)
{
    const auto data = QByteArray::fromHex(ANIMATED_PNG_HEX);
    const auto withExif = addExifToPng(data);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromFile("animated.png", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // EXIF is removed without re-encoding the animation.
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "apng");
}

TEST_F(ImageUploaderInputTest, KeepsAnimatedApngFile)
{
    const auto data = QByteArray::fromHex(ANIMATED_PNG_HEX);
    auto [images, error] = this->imagesFromFile("animated.apng", data);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "apng");
}

TEST_F(ImageUploaderInputTest, StripsExifFromAnimatedWebpFile)
{
    const auto data = QByteArray::fromHex(ANIMATED_WEBP_HEX);
    const auto withExif = addExifToWebp(data);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromFile("animated.webp", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // EXIF is removed without re-encoding the animation.
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "webp");
}

TEST_F(ImageUploaderInputTest, StripsExifWhenEncodingStaticWebpFile)
{
    const auto data = QByteArray::fromHex(STATIC_WEBP_HEX);
    const auto withExif = addExifToWebp(data);
    ASSERT_TRUE(withExif.contains("forsen"));
    ASSERT_FALSE(QImage::fromData(withExif, "WEBP").isNull());

    auto [images, error] = this->imagesFromFile("static.webp", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // Static WebP files are re-encoded as PNG without EXIF.
    EXPECT_EQ(images.front().format, "png");
    EXPECT_FALSE(images.front().data.contains("forsen"));
    EXPECT_FALSE(QImage::fromData(images.front().data, "PNG").isNull());
}

TEST_F(ImageUploaderInputTest, StripsExifFromAnimatedPngClipboard)
{
    const auto data = QByteArray::fromHex(ANIMATED_PNG_HEX);
    const auto withExif = addExifToPng(data);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromClipboard("image/apng", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // EXIF is removed without re-encoding the animation.
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "apng");
}

TEST_F(ImageUploaderInputTest, StripsExifFromAnimatedWebpClipboard)
{
    const auto data = QByteArray::fromHex(ANIMATED_WEBP_HEX);
    const auto withExif = addExifToWebp(data);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromClipboard("image/webp", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // EXIF is removed without re-encoding the animation.
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "webp");
}

TEST_F(ImageUploaderInputTest, StripsExifWhenEncodingJpegFile)
{
    const auto jpeg = makeImage("JPEG");
    const auto withExif = addExifToJpeg(jpeg);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromFile("image.jpg", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // JPEG files are re-encoded as PNG without EXIF.
    EXPECT_EQ(images.front().format, "png");
    EXPECT_FALSE(images.front().data.contains("forsen"));
    EXPECT_FALSE(QImage::fromData(images.front().data, "PNG").isNull());
}

TEST_F(ImageUploaderInputTest, StripsExifWhenEncodingPngFile)
{
    const auto png = makeImage("PNG");
    const auto withExif = addExifToPng(png);
    ASSERT_TRUE(withExif.contains("forsen"));
    ASSERT_FALSE(QImage::fromData(withExif, "PNG").isNull());

    auto [images, error] = this->imagesFromFile("image.png", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // PNG files are re-encoded without EXIF.
    EXPECT_EQ(images.front().format, "png");
    EXPECT_FALSE(images.front().data.contains("forsen"));
    EXPECT_FALSE(QImage::fromData(images.front().data, "PNG").isNull());
}

TEST_F(ImageUploaderInputTest, StripsExifFromJpegClipboard)
{
    const auto data = makeImage("JPEG");
    const auto withExif = addExifToJpeg(data);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromClipboard("image/jpeg", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // EXIF is removed without re-encoding the image.
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "jpeg");
}

TEST_F(ImageUploaderInputTest, StripsExifFromPngClipboard)
{
    const auto data = makeImage("PNG");
    const auto withExif = addExifToPng(data);
    ASSERT_TRUE(withExif.contains("forsen"));

    auto [images, error] = this->imagesFromClipboard("image/png", withExif);
    ASSERT_TRUE(error.isEmpty()) << error.toStdString();
    ASSERT_EQ(images.size(), 1);

    // EXIF is removed without re-encoding the image.
    EXPECT_EQ(images.front().data, data);
    EXPECT_EQ(images.front().format, "png");
}

namespace chatterino::imageuploader::detail {

TEST(ImageUploaderDetail_GetJsonValue, object)
{
    const char *json = R"({
        "foo": "bar",
        "baz": {
            "qox": {
                "a": 1,
                "b": "bb",
                "c": null,
                "d": true,
                "1": "one"
            }
        }
    })";
    auto jv = QJsonDocument::fromJson(json).object();
    ASSERT_FALSE(jv.empty());

    std::vector<std::pair<QString, QString>> cases{
        {"foo", "bar"},
        {"something", ""},
        {"0", ""},
        {"baz", ""},
        {"baz.qox", ""},
        {"baz.something", ""},
        {"baz.qox.a", ""},
        {"baz.qox.a.b", ""},
        {"baz.qox.a.b.c", ""},
        {"baz.qox.b", "bb"},
        {"baz.qox.b.1", "bb"},
        {"baz.qox.b.a.whatever", "bb"},
        {"baz.qox.c", ""},
        {"baz.qox.c.b", ""},
        {"baz.qox.c.d.e", ""},
        {"baz.qox.d", ""},
        {"baz.qox.d.f", ""},
        {"baz.qox.1", "one"},
        {"baz.qox.1.2", "one"},
        {"baz.qox.2", ""},
        {"baz.qox.2.4", ""},
        {"baz.qox.something", ""},
        {"baz.qox.something.else", ""},
        {"baz.qox.something.else.no", ""},
    };

    for (const auto &[pattern, exp] : cases)
    {
        ASSERT_EQ(getJSONValue(jv, pattern), exp) << pattern;
    }
}

TEST(ImageUploaderDetail_GetJsonValue, array)
{
    const char *json = R"([
        "bar",
        2,
        true,
        "f",
        null,
        {
            "qox": {
                "b": "bb"
            },
            "nest": [
                ["a", "b", ["c"]],
                [42, "d"]
            ]
        },
        ["x"]
    ])";
    auto jv = QJsonDocument::fromJson(json).array();
    ASSERT_FALSE(jv.empty());

    std::vector<std::pair<QString, QString>> cases{
        {"foo", ""},
        {"-1", ""},
        {"0x0", ""},
        {"0", "bar"},
        {"0.", "bar"},
        {"0.0", "bar"},
        {"1", ""},
        {"2", ""},
        {"3", "f"},
        {"4", ""},
        {"5", ""},
        {"5.0", ""},
        {"5.qox", ""},
        {"5.qox.b", "bb"},
        {"5.qox.0", ""},
        {"5.nest", ""},
        {"5.nest.0", ""},
        {"5.nest.0.0", "a"},
        {"5.nest.0.1", "b"},
        {"5.nest.0.2", ""},
        {"5.nest.0.2.0", "c"},
        {"5.nest.0.2.0.1", "c"},
        {"5.nest.0.2.1", ""},
        {"5.nest.0.3", ""},
        {"5.nest.0.3.0", ""},
        {"5.nest.0.3.0.0", ""},
        {"5.nest.1", ""},
        {"5.nest.1.0", ""},
        {"5.nest.1.1", "d"},
        {"5.nest.1.1.0", "d"},
        {"5.nest.1.2", ""},
        {"6.0", "x"},
        {"6.zero", ""},
        {"6.-1", ""},
        {"6.+0", "x"},
        {"7", ""},
        {"7.0", ""},
        {"8", ""},
    };

    for (const auto &[pattern, exp] : cases)
    {
        ASSERT_EQ(getJSONValue(jv, pattern), exp) << pattern;
    }
}

TEST(ImageUploaderDetail_GetJsonValue, scalar)
{
    ASSERT_EQ(getJSONValue({}, u""), "");
    ASSERT_EQ(getJSONValue({}, u"a"), "");
    ASSERT_EQ(getJSONValue({}, u"0"), "");
    ASSERT_EQ(getJSONValue({}, u"a.b"), "");

    ASSERT_EQ(getJSONValue(QJsonValue::Null, u""), "");
    ASSERT_EQ(getJSONValue(QJsonValue::Null, u"a"), "");
    ASSERT_EQ(getJSONValue(QJsonValue::Null, u"0"), "");
    ASSERT_EQ(getJSONValue(QJsonValue::Null, u"a.b"), "");

    ASSERT_EQ(getJSONValue(true, u""), "");
    ASSERT_EQ(getJSONValue(true, u"a"), "");
    ASSERT_EQ(getJSONValue(true, u"0"), "");
    ASSERT_EQ(getJSONValue(true, u"a.b"), "");

    ASSERT_EQ(getJSONValue(42, u""), "");
    ASSERT_EQ(getJSONValue(42, u"a"), "");
    ASSERT_EQ(getJSONValue(42, u"0"), "");
    ASSERT_EQ(getJSONValue(42, u"a.b"), "");

    ASSERT_EQ(getJSONValue("abc", u""), "abc");
    ASSERT_EQ(getJSONValue("abc", u"a"), "abc");
    ASSERT_EQ(getJSONValue("abc", u"0"), "abc");
    ASSERT_EQ(getJSONValue("abc", u"a.b"), "abc");
}

TEST(ImageUploaderDetail_GetLinkFromResponse, basic)
{
    const char *json = R"({
        "foo": "bar",
        "baz": {
            "qox": {
                "a": 1,
                "b": "bb",
                "c": null,
                "d": true,
                "1": "one"
            },
            "arr": [
                ["a", "b", ["c"]],
                [42, "d", "baz"],
                {
                    "a": "wow"
                }
            ]
        }
    })";

    std::vector<std::pair<QString, QString>> cases{
        {"a", "a"},
        {"foo", "foo"},
        {"{foo}", "bar"},
        {"foo{foo}", "foobar"},
        {"foo.{baz.qox}", "foo."},
        {"foo.{baz.qox.b}", "foo.bb"},
        {"{foo", "{foo"},
        {"foo}", "foo}"},
        {"{}", "{}"},
        {"{.}", ""},
        {"f{..}g", "fg"},
        {"{baz.qox.b}", "bb"},
        {"{baz.qox.{baz.arr.0.1}}", "}"},
        {"a{foo}b{foo}c{baz.arr.0.0}", "abarbbarca"},
        {".{foo}.{foo}.{baz.arr.0.0}.", ".bar.bar.a."},
        {"https://{foo}.{foo}/{baz.arr.0.0}.png", "https://bar.bar/a.png"},
    };

    NetworkResult res(NetworkResult::NetworkError::NoError, 200, json);
    for (const auto &[pattern, exp] : cases)
    {
        ASSERT_EQ(getLinkFromResponse(res, pattern), exp) << pattern;
    }
}

TEST(ImageUploaderDetail_GetLinkFromResponse, scalar)
{
    auto res = [](const char *json) {
        return NetworkResult(NetworkResult::NetworkError::NoError, 200, json);
    };

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{}-"), "-{}-");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{.}-"),
              "-my string-");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{0}-"),
              "-my string-");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{a}-"),
              "-my string-");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{a.b}-"),
              "-my string-");
#else
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{}-"), "-{}-");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{.}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{0}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{a}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res(R"("my string")"), "-{a.b}-"), "--");
#endif

    ASSERT_EQ(getLinkFromResponse(res("my string"), "-{}-"), "-{}-");
    ASSERT_EQ(getLinkFromResponse(res("my string"), "-{.}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("my string"), "-{0}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("my string"), "-{a}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("my string"), "-{a.b}-"), "--");

    ASSERT_EQ(getLinkFromResponse(res("42"), "-{}-"), "-{}-");
    ASSERT_EQ(getLinkFromResponse(res("42"), "-{.}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("42"), "-{0}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("42"), "-{a}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("42"), "-{a.b}-"), "--");

    ASSERT_EQ(getLinkFromResponse(res("true"), "-{}-"), "-{}-");
    ASSERT_EQ(getLinkFromResponse(res("true"), "-{.}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("true"), "-{0}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("true"), "-{a}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("true"), "-{a.b}-"), "--");

    ASSERT_EQ(getLinkFromResponse(res("null"), "-{}-"), "-{}-");
    ASSERT_EQ(getLinkFromResponse(res("null"), "-{.}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("null"), "-{0}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("null"), "-{a}-"), "--");
    ASSERT_EQ(getLinkFromResponse(res("null"), "-{a.b}-"), "--");
}

class ImageUploaderTest : public ::testing::Test
{
protected:
    void configure()
    {
        this->app = std::make_unique<mock::BaseApplication>();
    }

    void TearDown() override
    {
        this->app.reset();
    }

    std::unique_ptr<mock::BaseApplication> app;
};

TEST_F(ImageUploaderTest, ImportClearsMissingOptionalSettings)
{
    this->configure();

    auto &settings = *getSettings();
    settings.imageUploaderHeaders = "Authorization: Bearer ugandan-key";
    settings.imageUploaderDeletionLink =
        "https://old.example/delete/{delete-key}";

    const QJsonObject importedSettings{
        {"Version", "1.0.0"},
        {"RequestURL", "https://new.example/upload"},
        {"FileFormName", "file"},
        {"URL", "{response}"},
    };

    ASSERT_TRUE(importSettings(importedSettings, settings));
    EXPECT_TRUE(settings.imageUploaderHeaders.getValue().isEmpty());
    EXPECT_TRUE(settings.imageUploaderDeletionLink.getValue().isEmpty());
}

class ImportTest : public ImageUploaderTest,
                   public ::testing::WithParamInterface<QString>
{
};
TEST_P(ImportTest, Run)
{
    auto fixture = testlib::Snapshot::read("ImageUploader/Import", GetParam());

    configure();

    detail::importSettings(fixture->input().toObject(), *getSettings());

    ASSERT_TRUE(fixture->run(
        QJsonObject{
            {"url", getSettings()->imageUploaderUrl.getValue()},
            {"formField", getSettings()->imageUploaderFormField.getValue()},
            {"headers", getSettings()->imageUploaderHeaders.getValue()},
            {"link", getSettings()->imageUploaderLink.getValue()},
            {"deletionLink",
             getSettings()->imageUploaderDeletionLink.getValue()},
        },
        UPDATE_SNAPSHOTS));
}

INSTANTIATE_TEST_SUITE_P(
    ImageUploader, ImportTest,
    testing::ValuesIn(testlib::Snapshot::discover("ImageUploader/Import")));

class ExportTest : public ImageUploaderTest,
                   public ::testing::WithParamInterface<QString>
{
};
TEST_P(ExportTest, Run)
{
    auto fixture = testlib::Snapshot::read("ImageUploader/Export", GetParam());

    configure();

    const auto input = fixture->input().toObject();
    getSettings()->imageUploaderUrl = input["url"].toString();
    getSettings()->imageUploaderFormField = input["formField"].toString();
    getSettings()->imageUploaderHeaders = input["headers"].toString();
    getSettings()->imageUploaderLink = input["link"].toString();
    getSettings()->imageUploaderDeletionLink = input["deletionLink"].toString();

    auto output = detail::exportSettings(*getSettings());

    ASSERT_TRUE(fixture->run(output, UPDATE_SNAPSHOTS));
}

INSTANTIATE_TEST_SUITE_P(
    ImageUploader, ExportTest,
    testing::ValuesIn(testlib::Snapshot::discover("ImageUploader/Export")));

// verify that all snapshots are included
TEST(ImageUploader, ImportExportIntegrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}

}  // namespace chatterino::imageuploader::detail
