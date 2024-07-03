#include "common/ChatterSet.hpp"

#include "Test.hpp"

#include <QStringList>

TEST(ChatterSet, insert)
{
    chatterino::ChatterSet set;

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
    chatterino::ChatterSet set;

    EXPECT_FALSE(set.contains("pajlada"));
    EXPECT_FALSE(set.contains("Pajlada"));

    set.addRecentChatter("pajlada");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // After adding CHATTER_LIMIT-1 additional chatters, pajlada should still be in the set
    for (auto i = 0; i < chatterino::ChatterSet::chatterLimit - 1; ++i)
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
    chatterino::ChatterSet set;

    EXPECT_FALSE(set.contains("pajlada"));
    EXPECT_FALSE(set.contains("Pajlada"));

    set.addRecentChatter("pajlada");
    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // After adding CHATTER_LIMIT-1 additional chatters, pajlada should still be in the set
    for (auto i = 0; i < chatterino::ChatterSet::chatterLimit - 1; ++i)
    {
        set.addRecentChatter(QString("%1").arg(i));
    }

    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));

    // Bump pajlada as a recent chatter
    set.addRecentChatter("pajlada");

    // After another CHATTER_LIMIT-1 additional chatters, pajlada should still be there
    for (auto i = 0; i < chatterino::ChatterSet::chatterLimit - 1; ++i)
    {
        set.addRecentChatter(QString("new-%1").arg(i));
    }

    EXPECT_TRUE(set.contains("pajlada"));
    EXPECT_TRUE(set.contains("Pajlada"));
}
