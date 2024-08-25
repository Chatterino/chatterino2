#include "common/ChatterSet.hpp"

#include "Test.hpp"

#include <QStringList>

using namespace chatterino;

TEST(ChatterSet, insert)
{
    ChatterSet set;

    EXPECT_FALSE(set.contains("pajlada"));
    EXPECT_FALSE(set.contains("Pajlada"));

    set.addRecentChatter("pajlada");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    set.addRecentChatter("pajlada");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    set.addRecentChatter("PAJLADA");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));
}

TEST(ChatterSet, MaxSize)
{
    ChatterSet set;

    EXPECT_FALSE(set.contains("pajlada"));
    EXPECT_FALSE(set.contains("Pajlada"));

    set.addRecentChatter("pajlada");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // After adding CHATTER_LIMIT-1 additional chatters, pajlada should still be in the set
    for (auto i = 0; i < ChatterSet::CHATTER_LIMIT - 1; ++i)
    {
        set.addRecentChatter(QString("%1").arg(i));
    }

    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // But adding one more chatter should bump pajlada out of the set
    set.addRecentChatter("notpajlada");

    EXPECT_FALSE(set.contains("pajlada"));
    EXPECT_FALSE(set.contains("Pajlada"));
}

TEST(ChatterSet, MaxSizeLastUsed)
{
    ChatterSet set;

    EXPECT_FALSE(set.contains("pajlada"));
    EXPECT_FALSE(set.contains("Pajlada"));

    set.addRecentChatter("pajlada");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // After adding CHATTER_LIMIT-1 additional chatters, pajlada should still be in the set
    for (auto i = 0; i < ChatterSet::CHATTER_LIMIT - 1; ++i)
    {
        set.addRecentChatter(QString("%1").arg(i));
    }

    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // Bump pajlada as a recent chatter
    set.addRecentChatter("pajlada");

    // After another CHATTER_LIMIT-1 additional chatters, pajlada should still be there
    for (auto i = 0; i < ChatterSet::CHATTER_LIMIT - 1; ++i)
    {
        set.addRecentChatter(QString("new-%1").arg(i));
    }

    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));
}
