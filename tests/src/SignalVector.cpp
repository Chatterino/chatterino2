// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "common/SignalVector.hpp"

#include "Test.hpp"

#include <tuple>
#include <vector>

using namespace chatterino;

TEST(SignalVector, InsertPublishesSnapshotBeforeNotification)
{
    SignalVector<int> vector;
    vector.append(1);
    vector.append(3);
    const auto previous = vector.readOnly();
    const std::vector<int> expected{1, 2, 3};
    bool notified = false;

    std::ignore = vector.itemInserted.connect([&](const auto &event) {
        notified = true;
        EXPECT_EQ(event.index, 1);
        EXPECT_EQ(vector.raw(), expected);
        EXPECT_EQ(*vector.readOnly(), expected);
    });

    vector.insert(2, 1);
    EXPECT_TRUE(notified);
    EXPECT_EQ(*previous, (std::vector<int>{1, 3}));
}

TEST(SignalVector, RemoveAtPublishesSnapshotBeforeNotification)
{
    SignalVector<int> vector;
    vector.append(1);
    vector.append(2);
    const auto previous = vector.readOnly();
    const std::vector<int> expected{1};
    bool notified = false;

    std::ignore = vector.itemRemoved.connect([&](const auto &event) {
        notified = true;
        EXPECT_EQ(event.index, 1);
        EXPECT_EQ(vector.raw(), expected);
        EXPECT_EQ(*vector.readOnly(), expected);
    });

    vector.removeAt(1);
    EXPECT_TRUE(notified);
    EXPECT_EQ(*previous, (std::vector<int>{1, 2}));
}

TEST(SignalVector, RemoveFirstMatchingPublishesSnapshotBeforeNotification)
{
    SignalVector<int> vector;
    vector.append(1);
    vector.append(2);
    vector.append(3);
    const auto previous = vector.readOnly();
    const std::vector<int> expected{1, 3};
    bool notified = false;

    std::ignore = vector.itemRemoved.connect([&](const auto &event) {
        notified = true;
        EXPECT_EQ(event.index, 1);
        EXPECT_EQ(vector.raw(), expected);
        EXPECT_EQ(*vector.readOnly(), expected);
    });

    EXPECT_TRUE(vector.removeFirstMatching([](int item) {
        return item == 2;
    }));
    EXPECT_TRUE(notified);
    EXPECT_EQ(*previous, (std::vector<int>{1, 2, 3}));
}
