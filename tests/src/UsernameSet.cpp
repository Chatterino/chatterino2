#include "common/UsernameSet.hpp"

#include <gtest/gtest.h>
#include <QStringList>

chatterino::Prefix prefix_pajlada(QString("pajlada"));
chatterino::Prefix prefix_Pajlada(QString("Pajlada"));
chatterino::Prefix prefix_randers(QString("randers"));
chatterino::Prefix prefix_Chancu(QString("ch"));

TEST(Prefix, isStartOf)
{
    EXPECT_TRUE(prefix_pajlada.isStartOf("pajlada"));
    EXPECT_TRUE(prefix_pajlada.isStartOf("Pajlada"));
    EXPECT_FALSE(prefix_pajlada.isStartOf("randers"));

    EXPECT_TRUE(prefix_Pajlada.isStartOf("pajlada"));
    EXPECT_TRUE(prefix_Pajlada.isStartOf("Pajlada"));
    EXPECT_TRUE(prefix_Pajlada.isStartOf("pajbot"));
    EXPECT_TRUE(prefix_Pajlada.isStartOf("Pajbot"));
    EXPECT_FALSE(prefix_Pajlada.isStartOf("randers"));
}

TEST(Prefix, EqualsOperator)
{
    EXPECT_EQ(prefix_pajlada, prefix_Pajlada);
    EXPECT_NE(prefix_pajlada, prefix_randers);
}

TEST(UsernameSet, insert)
{
    std::pair<chatterino::UsernameSet::Iterator, bool> p;
    chatterino::UsernameSet set;

    EXPECT_EQ(set.size(), 0);

    p = set.insert("pajlada");
    EXPECT_TRUE(p.second);

    EXPECT_EQ(set.size(), 1);

    p = set.insert("pajlada");
    EXPECT_FALSE(p.second);

    EXPECT_EQ(set.size(), 1);

    p = set.insert("pajbot");
    EXPECT_TRUE(p.second);

    EXPECT_EQ(set.size(), 2);

    p = set.insert("pajlada");
    EXPECT_FALSE(p.second);

    EXPECT_EQ(set.size(), 2);

    p = set.insert("PAJLADA");
    EXPECT_FALSE(p.second);

    EXPECT_EQ(set.size(), 2);
}

TEST(UsernameSet, CollisionTest)
{
    QString s;
    chatterino::UsernameSet set;
    chatterino::Prefix prefix("not_");

    set.insert("pajlada");
    set.insert("Chancu");
    set.insert("chief_tony");
    set.insert("ChodzacyKac");
    set.insert("ChatAbuser");
    set.insert("Normies_GTFO");
    set.insert("not_remzy");
    set.insert("Mullo2500");
    set.insert("muggedbyapie");

    EXPECT_EQ(set.size(), 9);

    {
        QStringList result;
        QStringList expectation{"Normies_GTFO", "not_remzy"};
        auto subrange = set.subrange(QString("not_"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }

    {
        QStringList result;
        QStringList expectation{};
        auto subrange = set.subrange(QString("te"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }

    {
        QStringList result;
        QStringList expectation{"pajlada"};
        auto subrange = set.subrange(QString("PA"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }

    {
        QStringList result;
        QStringList expectation{"pajlada"};
        auto subrange = set.subrange(QString("pajlada"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }

    {
        QStringList result;
        QStringList expectation{"pajlada"};
        auto subrange = set.subrange(QString("Pajl"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }

    {
        QStringList result;
        QStringList expectation{"Chancu", "ChatAbuser", "chief_tony",
                                "ChodzacyKac"};
        auto subrange = set.subrange(QString("chan"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }

    {
        QStringList result;
        QStringList expectation{"muggedbyapie", "Mullo2500"};
        auto subrange = set.subrange(QString("mu"));
        std::copy(subrange.begin(), subrange.end(), std::back_inserter(result));
        EXPECT_EQ(expectation, result);
    }
}

