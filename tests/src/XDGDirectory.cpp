#include "util/XDGDirectory.hpp"

#include "Test.hpp"
#include "util/CombinePath.hpp"

#include <QDebug>
#include <QTemporaryDir>
#include <QtEnvironmentVariables>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

using namespace chatterino;

namespace {

class TempEnv
{
public:
    TempEnv(const char *key_, const QString &value_)
        : key(key_)
        , value(value_)
        , prevValue(qEnvironmentVariable(this->key))
    {
        if (!this->value.isNull())
        {
            qDebug() << "Setting" << qPrintable(this->key) << "from"
                     << this->prevValue << "to" << this->value;
            qputenv(this->key, this->value.toLocal8Bit());
        }
        else
        {
            qDebug() << "Unsetting" << qPrintable(this->key)
                     << "- old value was" << this->prevValue;
            qunsetenv(this->key);
        }
    }

    TempEnv(const TempEnv &) = delete;
    TempEnv(TempEnv &&) = delete;
    TempEnv &operator=(const TempEnv &) = delete;
    TempEnv &operator=(TempEnv &&) = delete;

    ~TempEnv()
    {
        if (!this->prevValue.isNull())
        {
            qDebug() << "Reverting" << qPrintable(this->key) << "to"
                     << this->prevValue;
            qputenv(this->key, this->prevValue.toLocal8Bit());
        }
        else
        {
            qDebug() << "Unsetting" << qPrintable(this->key) << "to"
                     << this->prevValue;
            qunsetenv(this->key);
        }
    }

    const QString &getValue() const
    {
        return this->value;
    }

private:
    const char *key;
    QString value;
    QString prevValue;
};

}  // namespace

/// Test the returned directories from XDGDirectoryType::Config when no extra environment variables are set
TEST(XDGDirectory, ConfigDefault)
{
    QTemporaryDir tmp;
    auto lock = environmentLock();

    TempEnv home("HOME", combinePath(tmp.path(), "home"));
    TempEnv dataHome("XDG_CONFIG_HOME", {});
    TempEnv dataDirs("XDG_CONFIG_DIRS", {});

    auto actual = getXDGDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(actual.at(0), combinePath(home.getValue(), ".config/"));
    ASSERT_EQ(actual.at(1), "/etc/xdg");

    ASSERT_EQ(actual.length(), 2);
}

TEST(XDGDirectory, ConfigCustom)
{
    auto lock = environmentLock();

    TempEnv dataHome("XDG_CONFIG_HOME", "/tmp/home-data");
    TempEnv dataDirs("XDG_CONFIG_DIRS", "/tmp/sys-data-1:/tmp/sys-data-2");

    auto actual = getXDGDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(actual.at(0), "/tmp/home-data");
    ASSERT_EQ(actual.at(1), "/tmp/sys-data-1");
    ASSERT_EQ(actual.at(2), "/tmp/sys-data-2");

    ASSERT_EQ(actual.length(), 3);
}

TEST(XDGDirectory, ConfigUserDefault)
{
    QTemporaryDir tmp;
    auto lock = environmentLock();

    TempEnv home("HOME", combinePath(tmp.path(), "home"));
    TempEnv dataHome("XDG_CONFIG_HOME", {});

    auto actual = getXDGUserDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(actual.at(0), combinePath(home.getValue(), ".config/"));

    ASSERT_EQ(actual.length(), 1);
}

TEST(XDGDirectory, ConfigUserCustom)
{
    auto lock = environmentLock();

    TempEnv dataDirs("XDG_CONFIG_HOME", "/tmp/home-data");

    auto actual = getXDGUserDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(actual.at(0), "/tmp/home-data");

    ASSERT_EQ(actual.length(), 1);
}

TEST(XDGDirectory, ConfigBaseDefault)
{
    auto lock = environmentLock();

    TempEnv dataDirs("XDG_CONFIG_DIRS", {});

    auto actual = getXDGBaseDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(actual.at(0), "/etc/xdg");

    ASSERT_EQ(actual.length(), 1);
}

TEST(XDGDirectory, ConfigBaseCustom)
{
    auto lock = environmentLock();

    TempEnv dataDirs("XDG_CONFIG_DIRS",
                     "/tmp/sys-data-1:/tmp/sys-data-2:/tmp/sys-data-3");

    auto actual = getXDGBaseDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(actual.at(0), "/tmp/sys-data-1");
    ASSERT_EQ(actual.at(1), "/tmp/sys-data-2");
    ASSERT_EQ(actual.at(2), "/tmp/sys-data-3");

    ASSERT_EQ(actual.length(), 3);
}

/// Test the returned directories from XDGDirectoryType::Data when no extra environment variables are set
TEST(XDGDirectory, DataDefault)
{
    QTemporaryDir tmp;
    auto lock = environmentLock();

    TempEnv home("HOME", combinePath(tmp.path(), "home"));
    TempEnv dataHome("XDG_DATA_HOME", {});
    TempEnv dataDirs("XDG_DATA_DIRS", {});

    auto actual = getXDGDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(actual.at(0), combinePath(home.getValue(), ".local/share/"));
    ASSERT_EQ(actual.at(1), "/usr/local/share/");
    ASSERT_EQ(actual.at(2), "/usr/share/");

    ASSERT_EQ(actual.length(), 3);
}

/// Test the returned directories from XDGDirectoryType::Data when user custom-configured environment variables are set
TEST(XDGDirectory, DataCustom)
{
    auto lock = environmentLock();

    TempEnv dataHome("XDG_DATA_HOME", "/tmp/home-share");
    TempEnv dataDirs("XDG_DATA_DIRS",
                     "/tmp/sys-share-1:/tmp/sys-share-2:/tmp/sys-share-3");

    auto actual = getXDGDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(actual.at(0), "/tmp/home-share");
    ASSERT_EQ(actual.at(1), "/tmp/sys-share-1");
    ASSERT_EQ(actual.at(2), "/tmp/sys-share-2");
    ASSERT_EQ(actual.at(3), "/tmp/sys-share-3");

    ASSERT_EQ(actual.length(), 4);
}

TEST(XDGDirectory, DataUserDefault)
{
    QTemporaryDir tmp;
    auto lock = environmentLock();

    TempEnv home("HOME", combinePath(tmp.path(), "home"));
    TempEnv dataDirs("XDG_DATA_HOME", {});

    auto actual = getXDGUserDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(actual.at(0), combinePath(home.getValue(), ".local/share/"));

    ASSERT_EQ(actual.length(), 1);
}

TEST(XDGDirectory, DataUserCustom)
{
    auto lock = environmentLock();

    TempEnv dataHome("XDG_DATA_HOME", "/tmp/home-share");

    auto actual = getXDGUserDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(actual.at(0), "/tmp/home-share");

    ASSERT_EQ(actual.length(), 1);
}

TEST(XDGDirectory, DataBaseDefault)
{
    auto lock = environmentLock();

    TempEnv dataDirs("XDG_DATA_DIRS", {});

    auto actual = getXDGBaseDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(actual.at(0), "/usr/local/share/");
    ASSERT_EQ(actual.at(1), "/usr/share/");

    ASSERT_EQ(actual.length(), 2);
}

TEST(XDGDirectory, DataBaseCustom)
{
    auto lock = environmentLock();

    TempEnv dataDirs("XDG_DATA_DIRS", "/tmp/sys-share-1");

    auto actual = getXDGBaseDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(actual.at(0), "/tmp/sys-share-1");

    ASSERT_EQ(actual.length(), 1);
}

#endif
