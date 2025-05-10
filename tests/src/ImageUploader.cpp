#include "singletons/ImageUploader.hpp"

#include "common/network/NetworkResult.hpp"
#include "Test.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <utility>
#include <vector>

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

}  // namespace chatterino::imageuploader::detail
