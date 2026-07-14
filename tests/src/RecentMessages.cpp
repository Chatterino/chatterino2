// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/recentmessages/Impl.hpp"
#include "providers/recentmessages/Provider.hpp"
#include "providers/recentmessages/ProviderModel.hpp"
#include "Test.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QUrlQuery>

#include <array>
#include <set>
#include <vector>

using namespace chatterino::recentmessages;

TEST(RecentMessages, DefaultProvidersAreValid)
{
    const auto providers = defaultProviders();

    ASSERT_FALSE(providers.empty());
    bool hasEnabledProvider = false;
    std::set<QString> ids;
    for (const auto &provider : providers)
    {
        EXPECT_TRUE(provider.isBuiltIn());
        EXPECT_FALSE(provider.id().isEmpty());
        EXPECT_TRUE(ids.insert(provider.id()).second);
        EXPECT_TRUE(provider.isValid()) << provider.url();
        hasEnabledProvider |= provider.enabled();
    }
    EXPECT_TRUE(hasEnabledProvider);
}

TEST(RecentMessages, ProviderValidation)
{
    struct TestCase {
        const char *url;
        bool expectedValid;
    };

    const std::array<TestCase, 6> cases{
        TestCase{.url = "https://example.com/recent-messages/%1",
                 .expectedValid = true},
        TestCase{.url = "http://localhost:8080/%1", .expectedValid = true},
        TestCase{.url = "https://example.com/recent-messages",
                 .expectedValid = false},
        TestCase{.url = "ftp://example.com/%1", .expectedValid = false},
        TestCase{.url = "https:///recent-messages/%1", .expectedValid = false},
        TestCase{.url = "not a URL %1", .expectedValid = false},
    };

    for (const auto &test : cases)
    {
        const Provider provider{test.url, true};
        EXPECT_EQ(provider.isValid(), test.expectedValid) << test.url;
    }
}

TEST(RecentMessages, ProviderSerialization)
{
    rapidjson::Document document;
    const std::array<Provider, 2> providers{
        Provider{"https://built-in.example/%1", false, "built-in"},
        Provider{"https://custom.example/%1", true},
    };

    for (const auto &input : providers)
    {
        const auto json =
            pajlada::Serialize<Provider>::get(input, document.GetAllocator());

        ASSERT_TRUE(json.IsObject());
        ASSERT_TRUE(json.HasMember("id"));
        ASSERT_TRUE(json.HasMember("url"));
        ASSERT_TRUE(json.HasMember("enabled"));
        EXPECT_EQ(QString::fromUtf8(json["id"].GetString()), input.id());
        EXPECT_EQ(QString::fromUtf8(json["url"].GetString()), input.url());
        EXPECT_EQ(json["enabled"].GetBool(), input.enabled());

        bool error = false;
        const auto output = pajlada::Deserialize<Provider>::get(json, &error);

        EXPECT_FALSE(error);
        EXPECT_EQ(output, input);
    }
}

TEST(RecentMessages, ReconcileUpdatesAddsAndRemovesBuiltIns)
{
    const Provider updatedBuiltIn{"https://current.example/%1", true,
                                  "existing"};
    const Provider addedBuiltIn{"https://new.example/%1", true, "new"};
    const std::vector<Provider> builtIns{updatedBuiltIn, addedBuiltIn};

    const Provider savedBuiltIn{"https://old.example/%1", false, "existing"};
    const Provider removedBuiltIn{"https://retired.example/%1", true,
                                  "retired"};
    // the custom provider has the same URL as the removed built-in provider
    // and should survive reconcile
    const Provider customProvider{"https://retired.example/%1", true};
    const std::vector<Provider> saved{
        savedBuiltIn,
        removedBuiltIn,
        customProvider,
    };

    const auto result = reconcileProviders(saved, builtIns);

    const Provider updatedButStillDisabled{updatedBuiltIn.url(), false,
                                           updatedBuiltIn.id()};
    const std::vector<Provider> expected{
        updatedButStillDisabled,
        addedBuiltIn,
        customProvider,
    };
    EXPECT_EQ(result, expected);
}

TEST(RecentMessages, ReconcileProvidersIsIdempotent)
{
    const Provider first{"https://first.example/%1", true, "first"};
    const Provider second{"https://second.example/%1", true, "second"};
    const Provider third{"https://third.example/%1", true, "third"};
    const Provider custom{"https://custom.example/%1", true};
    const std::vector<Provider> builtIns{first, second, third};
    const std::vector<Provider> saved{first, custom, second, third};

    const auto result = reconcileProviders(saved, builtIns);

    EXPECT_EQ(result, saved);
}

TEST(RecentMessages, ReconcileNewBuiltInsInDefaultOrder)
{
    const Provider first{"https://first.example/%1", true, "first"};
    const Provider second{"https://second.example/%1", true, "second"};
    const Provider third{"https://third.example/%1", true, "third"};
    const Provider fourth{"https://fourth.example/%1", true, "fourth"};
    const Provider fifth{"https://fifth.example/%1", true, "fifth"};
    const Provider custom{"https://custom.example/%1", true};
    const std::vector<Provider> builtIns{first, second, third, fourth, fifth};

    // second and fourth are still in their default order, so missing
    // built-ins should be restored to their default positions
    const std::vector<Provider> saved{second, custom, fourth};

    const auto result = reconcileProviders(saved, builtIns);

    const std::vector<Provider> expected{first, second, custom,
                                         third, fourth, fifth};
    EXPECT_EQ(result, expected);
}

TEST(RecentMessages, ReconcileNewBuiltInsAfterCustomizedOrder)
{
    const Provider first{"https://first.example/%1", true, "first"};
    const Provider second{"https://second.example/%1", true, "second"};
    const Provider third{"https://third.example/%1", true, "third"};
    const Provider fourth{"https://fourth.example/%1", true, "fourth"};
    const Provider fifth{"https://fifth.example/%1", true, "fifth"};
    const Provider custom{"https://custom.example/%1", true};
    const std::vector<Provider> builtIns{first, second, third, fourth, fifth};

    // third, first, second is a custom order, so missing built-ins should be
    // appended instead of interfering with that order
    const std::vector<Provider> saved{third, first, custom, second};

    const auto result = reconcileProviders(saved, builtIns);

    const std::vector<Provider> expected{third,  first,  custom,
                                         second, fourth, fifth};
    EXPECT_EQ(result, expected);
}

TEST(RecentMessages, ProviderModelBuiltInPermissions)
{
    chatterino::SignalVector<Provider> providers;
    providers.append({"https://built-in.example/%1", true, "built-in"});

    ProviderModel model(nullptr);
    model.initialize(&providers);

    // enabled can be changed, but the URL cannot
    EXPECT_TRUE(model.flags(model.index(0, 0)) & Qt::ItemIsUserCheckable);
    EXPECT_FALSE(model.flags(model.index(0, 1)) & Qt::ItemIsEditable);

    EXPECT_TRUE(
        model.setData(model.index(0, 0), Qt::Unchecked, Qt::CheckStateRole));
    EXPECT_FALSE(providers.raw()[0].enabled());
    EXPECT_EQ(providers.raw()[0].id(), "built-in");

    // built-ins cannot be removed
    EXPECT_FALSE(model.removeRow(0));
    EXPECT_EQ(providers.raw().size(), 1);
}

TEST(RecentMessages, ProviderModelCustomPermissions)
{
    chatterino::SignalVector<Provider> providers;
    providers.append({"https://custom.example/%1", true});

    ProviderModel model(nullptr);
    model.initialize(&providers);

    EXPECT_TRUE(model.flags(model.index(0, 1)) & Qt::ItemIsEditable);
    EXPECT_TRUE(model.setData(model.index(0, 1), "https://changed.example/%1",
                              Qt::DisplayRole));
    EXPECT_EQ(providers.raw()[0].url(), "https://changed.example/%1");
    EXPECT_FALSE(providers.raw()[0].isBuiltIn());
    EXPECT_TRUE(model.removeRow(0));
    EXPECT_TRUE(providers.raw().empty());
}

TEST(RecentMessages, ProviderSelectionSkipsDisabledProviders)
{
    const std::vector<Provider> providers{
        {"https://disabled.example/%1", false},
        {"https://invalid.example/no-placeholder", true},
        {"https://first.example/%1", true},
        {"https://second.example/%1", true},
    };
    std::size_t index = 0;

    const auto invalid = nextEnabledProvider(providers, index);
    EXPECT_EQ(invalid,
              Provider("https://invalid.example/no-placeholder", true));

    const auto first = nextEnabledProvider(providers, index);
    EXPECT_EQ(first, Provider("https://first.example/%1", true));

    const auto second = nextEnabledProvider(providers, index);
    EXPECT_EQ(second, Provider("https://second.example/%1", true));

    EXPECT_FALSE(nextEnabledProvider(providers, index).has_value());
    EXPECT_EQ(index, providers.size());
}

TEST(RecentMessages, ConstructUrl)
{
    const auto url = detail::constructRecentMessagesUrl(
        "https://example.com/api/v2/recent-messages/%1?source=test&limit=20",
        "forsen", 100, std::nullopt, std::nullopt);
    const QUrlQuery query(url);

    EXPECT_EQ(url.path(), "/api/v2/recent-messages/forsen");
    EXPECT_EQ(query.queryItemValue("source"), "test");
    EXPECT_EQ(query.queryItemValue("limit"), "20");
}

TEST(RecentMessages, ResponseClassification)
{
    struct TestCase {
        const char *description;
        QJsonObject response;
        detail::ResponseType expectedType;
    };

    const std::array<TestCase, 9> cases{
        TestCase{.description = "empty messages array",
                 .response = QJsonObject{{"messages", QJsonArray{}}},
                 .expectedType = detail::ResponseType::Complete},
        TestCase{.description = "messages with null error fields",
                 .response = QJsonObject{{"messages", QJsonArray{"message"}},
                                         {"error", QJsonValue::Null},
                                         {"error_code", QJsonValue::Null}},
                 .expectedType = detail::ResponseType::Complete},
        TestCase{.description = "API error code",
                 .response = QJsonObject{{"messages", QJsonArray{}},
                                         {"error_code", "unrelated_error"}},
                 .expectedType = detail::ResponseType::Invalid},
        TestCase{.description = "provider is not joined and has no messages",
                 .response = QJsonObject{{"messages", QJsonArray{}},
                                         {"error", "The bot is not joined"},
                                         {"error_code", "channel_not_joined"}},
                 .expectedType = detail::ResponseType::Invalid},
        TestCase{
            .description = "provider is not joined but has partial history",
            .response = QJsonObject{{"messages", QJsonArray{"message"}},
                                    {"error", "The bot is not joined"},
                                    {"error_code", "channel_not_joined"}},
            .expectedType = detail::ResponseType::Partial},
        TestCase{.description = "API error message",
                 .response = QJsonObject{{"messages", QJsonArray{}},
                                         {"error", "Provider error"}},
                 .expectedType = detail::ResponseType::Invalid},
        TestCase{.description = "missing messages field",
                 .response = QJsonObject{},
                 .expectedType = detail::ResponseType::Invalid},
        TestCase{.description = "messages field is not an array",
                 .response = QJsonObject{{"messages", "not an array"}},
                 .expectedType = detail::ResponseType::Invalid},
        TestCase{
            .description = "messages array contains a non-string value",
            .response = QJsonObject{{"messages", QJsonArray{QJsonObject{}}}},
            .expectedType = detail::ResponseType::Invalid},
    };

    for (const auto &test : cases)
    {
        EXPECT_EQ(detail::classifyRecentMessagesResponse(test.response),
                  test.expectedType)
            << test.description;
    }
}
