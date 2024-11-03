#include "messages/LimitedQueue.hpp"

#include "Test.hpp"

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
    LimitedQueue<int> queue(10);
    queue.pushBack(1);
    queue.pushBack(2);
    queue.pushBack(3);
    queue.pushBack(4);
    queue.pushBack(5);
    queue.pushBack(6);

    int idex = queue.replaceItem(2, 10);
    EXPECT_EQ(idex, 1);
    idex = queue.replaceItem(7, 11);
    EXPECT_EQ(idex, -1);

    int prev = -1;
    bool res = queue.replaceItem(std::size_t(0), 9, &prev);
    EXPECT_TRUE(res);
    EXPECT_EQ(prev, 1);
    res = queue.replaceItem(std::size_t(6), 4);
    EXPECT_FALSE(res);

    // correct hint
    EXPECT_EQ(queue.replaceItem(3, 4, 11), 3);
    // incorrect hints
    EXPECT_EQ(queue.replaceItem(5, 11, 12), 3);
    EXPECT_EQ(queue.replaceItem(0, 12, 13), 3);
    // oob hint
    EXPECT_EQ(queue.replaceItem(42, 13, 14), 3);
    // bad needle
    EXPECT_EQ(queue.replaceItem(0, 15, 16), -1);

    SNAPSHOT_EQUALS(queue.getSnapshot(), {9, 10, 3, 14, 5, 6},
                    "first snapshot");
}

TEST(LimitedQueue, Find)
{
    LimitedQueue<int> queue(10);
    queue.pushBack(1);
    queue.pushBack(2);
    queue.pushBack(3);
    queue.pushBack(4);
    queue.pushBack(5);
    queue.pushBack(6);

    // without hint
    EXPECT_FALSE(queue
                     .find([](int i) {
                         return i == 0;
                     })
                     .has_value());
    EXPECT_EQ(queue
                  .find([](int i) {
                      return i == 1;
                  })
                  .value(),
              1);
    EXPECT_EQ(queue
                  .find([](int i) {
                      return i == 2;
                  })
                  .value(),
              2);
    EXPECT_EQ(queue
                  .find([](int i) {
                      return i == 6;
                  })
                  .value(),
              6);
    EXPECT_FALSE(queue
                     .find([](int i) {
                         return i == 7;
                     })
                     .has_value());
    EXPECT_FALSE(queue
                     .find([](int i) {
                         return i > 6;
                     })
                     .has_value());
    EXPECT_FALSE(queue
                     .find([](int i) {
                         return i <= 0;
                     })
                     .has_value());

    using Pair = std::pair<size_t, int>;
    // with hint
    EXPECT_FALSE(queue
                     .find(0,
                           [](int i) {
                               return i == 0;
                           })
                     .has_value());
    // correct hint
    EXPECT_EQ(queue
                  .find(0,
                        [](int i) {
                            return i == 1;
                        })
                  .value(),
              (Pair{0, 1}));
    EXPECT_EQ(queue
                  .find(1,
                        [](int i) {
                            return i == 2;
                        })
                  .value(),
              (Pair{1, 2}));
    // incorrect hint
    EXPECT_EQ(queue
                  .find(1,
                        [](int i) {
                            return i == 1;
                        })
                  .value(),
              (Pair{0, 1}));
    EXPECT_EQ(queue
                  .find(5,
                        [](int i) {
                            return i == 6;
                        })
                  .value(),
              (Pair{5, 6}));
    // oob hint
    EXPECT_EQ(queue
                  .find(6,
                        [](int i) {
                            return i == 3;
                        })
                  .value(),
              (Pair{2, 3}));
    // non-existent items
    EXPECT_FALSE(queue
                     .find(42,
                           [](int i) {
                               return i == 7;
                           })
                     .has_value());
    EXPECT_FALSE(queue
                     .find(0,
                           [](int i) {
                               return i > 6;
                           })
                     .has_value());
    EXPECT_FALSE(queue
                     .find(0,
                           [](int i) {
                               return i <= 0;
                           })
                     .has_value());
}
