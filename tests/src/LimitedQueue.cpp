#include "messages/LimitedQueue.hpp"

#include <gtest/gtest.h>
#include <vector>

using namespace chatterino;

namespace chatterino {

template <typename T>
std::ostream &operator<<(std::ostream &os,
                         const LimitedQueueSnapshot<T> &snapshot)
{
    os << "[ ";
    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        os << snapshot[i] << ' ';
    }
    os << "]";

    return os;
}

}  // namespace chatterino

namespace std {
template <typename T>
std::ostream &operator<<(std::ostream &os, const vector<T> &vec)
{
    os << "[ ";
    for (const auto &item : vec)
    {
        os << item << ' ';
    }
    os << "]";

    return os;
}
}  // namespace std

template <typename T>
inline void SNAPSHOT_EQUALS(const LimitedQueueSnapshot<T> &snapshot,
                            const std::vector<T> &values,
                            const std::string &msg)
{
    SCOPED_TRACE(msg);
    ASSERT_EQ(snapshot.size(), values.size())
        << "snapshot = " << snapshot << " values = " << values;

    if (snapshot.size() != values.size())
        return;

    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        EXPECT_EQ(snapshot[i], values[i]) << "i = " << i;
    }
}

TEST(LimitedQueue, PushBack)
{
    LimitedQueue<int> queue(5);
    int d = 0;
    bool flag;

    EXPECT_TRUE(queue.empty());
    flag = queue.pushBack(1, d);
    EXPECT_FALSE(flag);
    flag = queue.pushBack(2, d);
    EXPECT_FALSE(flag);

    EXPECT_FALSE(queue.empty());

    auto snapshot1 = queue.getSnapshot();
    SNAPSHOT_EQUALS(snapshot1, {1, 2}, "first snapshot");

    flag = queue.pushBack(3, d);
    EXPECT_FALSE(flag);
    flag = queue.pushBack(4, d);
    EXPECT_FALSE(flag);

    // snapshot should be the same
    SNAPSHOT_EQUALS(snapshot1, {1, 2}, "first snapshot same 1");

    flag = queue.pushBack(5, d);
    EXPECT_FALSE(flag);
    flag = queue.pushBack(6, d);
    EXPECT_TRUE(flag);
    EXPECT_EQ(d, 1);

    SNAPSHOT_EQUALS(snapshot1, {1, 2}, "first snapshot same 2");

    auto snapshot2 = queue.getSnapshot();
    SNAPSHOT_EQUALS(snapshot2, {2, 3, 4, 5, 6}, "second snapshot");
    SNAPSHOT_EQUALS(snapshot1, {1, 2}, "first snapshot same 3");
}

TEST(LimitedQueue, PushFront)
{
    LimitedQueue<int> queue(5);
    queue.pushBack(1);
    queue.pushBack(2);
    queue.pushBack(3);

    std::vector<int> expectedPush = {7, 8};
    auto pushed = queue.pushFront({4, 5, 6, 7, 8});
    auto snapshot = queue.getSnapshot();
    SNAPSHOT_EQUALS(snapshot, {7, 8, 1, 2, 3}, "first snapshot");
    EXPECT_EQ(pushed, expectedPush);

    auto pushed2 = queue.pushFront({9, 10, 11});
    EXPECT_EQ(pushed2.size(), 0);
}

TEST(LimitedQueue, ReplaceItem)
{
    LimitedQueue<int> queue(5);
    queue.pushBack(1);
    queue.pushBack(2);
    queue.pushBack(3);

    int idex = queue.replaceItem(2, 10);
    EXPECT_EQ(idex, 1);
    idex = queue.replaceItem(5, 11);
    EXPECT_EQ(idex, -1);

    bool res = queue.replaceItem(std::size_t(0), 9);
    EXPECT_TRUE(res);
    res = queue.replaceItem(std::size_t(5), 4);
    EXPECT_FALSE(res);

    SNAPSHOT_EQUALS(queue.getSnapshot(), {9, 10, 3}, "first snapshot");
}
