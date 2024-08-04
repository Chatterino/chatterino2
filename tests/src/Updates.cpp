#include "singletons/Updates.hpp"

#include "common/Version.hpp"
#include "Test.hpp"

#include <semver/semver.hpp>

using namespace chatterino;

TEST(Updates, MustBeDowngrade)
{
    EXPECT_TRUE(Updates::isDowngradeOf("1.0.0", "2.4.5"))
        << "1.0.0 must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.0.0", "2.4.5"))
        << "2.0.0 must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.0", "2.4.5"))
        << "2.4.0 must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.4-beta", "2.4.5"))
        << "2.4.4-beta must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta", "2.4.5"))
        << "2.4.5-beta must be a downgrade of 2.4.5";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta.1", "2.4.5-beta.2"))
        << "2.4.5-beta.1 must be a downgrade of 2.4.5-beta.2";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta", "2.4.5-beta.2"))
        << "2.4.5-beta must be a downgrade of 2.4.5-beta.2";
    EXPECT_TRUE(Updates::isDowngradeOf("2.4.5-beta.2", "2.4.6-beta.1"))
        << "2.4.5-beta.2 must be a downgrade of 2.4.6-beta.1";
}

TEST(Updates, MustNotBeDowngrade)
{
    EXPECT_FALSE(Updates::isDowngradeOf("2.4.5", "2.4.5"))
        << "2.4.5 must not be a downgrade of 2.4.5";
    EXPECT_FALSE(Updates::isDowngradeOf("2.4.5", "2.4.5-beta"))
        << "2.4.5 must not be a downgrade of 2.4.5-beta";
}

TEST(Updates, ValidateCurrentVersion)
{
    EXPECT_NO_THROW([[maybe_unused]] auto v = semver::from_string(
                        Version::instance().version().toStdString()))
        << "Current version must be valid semver";
    EXPECT_EQ(Version::instance().version(), CHATTERINO_VERSION);
}
