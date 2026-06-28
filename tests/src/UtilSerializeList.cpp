// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "Test.hpp"
#include "util/RapidJsonSerializeQString.hpp"  // IWYU pragma: keep
#include "util/serialize/List.hpp"             // IWYU pragma: keep

#include <QDebug>
#include <QString>

TEST(UtilSerializeList, String)
{
    rapidjson::Document d;
    QStringList in{"a", "b", "c"};
    auto middle = pajlada::Serialize<QStringList>::get(in, d.GetAllocator());
    ASSERT_TRUE(middle.IsArray());
    auto arr = middle.GetArray();
    ASSERT_EQ(arr.Size(), 3);
    ASSERT_EQ(arr[0], "a");
    ASSERT_EQ(arr[1], "b");
    ASSERT_EQ(arr[2], "c");

    bool error = false;
    auto out = pajlada::Deserialize<QStringList>::get(middle, &error);
    ASSERT_FALSE(error);
    ASSERT_EQ(out.length(), 3);
    ASSERT_EQ(in, out);
}

TEST(UtilSerializeList, StringEmpty)
{
    rapidjson::Document d;
    QStringList in{};
    auto middle = pajlada::Serialize<QStringList>::get(in, d.GetAllocator());
    ASSERT_TRUE(middle.IsArray());
    auto arr = middle.GetArray();
    ASSERT_EQ(arr.Size(), 0);

    bool error = false;
    auto out = pajlada::Deserialize<QStringList>::get(middle, &error);
    ASSERT_FALSE(error);
    ASSERT_EQ(out.length(), 0);
    ASSERT_EQ(in, out);
}

TEST(UtilSerializeList, Int)
{
    rapidjson::Document d;
    QList<int> in{69, 420};
    auto middle = pajlada::Serialize<QList<int>>::get(in, d.GetAllocator());
    ASSERT_TRUE(middle.IsArray());
    auto arr = middle.GetArray();
    ASSERT_EQ(arr.Size(), 2);
    ASSERT_EQ(arr[0], 69);
    ASSERT_EQ(arr[1], 420);

    bool error = false;
    auto out = pajlada::Deserialize<QList<int>>::get(middle, &error);
    ASSERT_FALSE(error);
    ASSERT_EQ(out.length(), 2);
    ASSERT_EQ(in, out);
}
