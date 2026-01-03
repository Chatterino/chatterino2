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
        if (this->value.isNull())
        {
            qDebug() << "Setting" << qPrintable(key) << "from"
                     << this->prevValue << "to" << this->value;
            qunsetenv(this->key);
        }
        else
        {
            qDebug() << "Unsetting" << qPrintable(key) << "- old value was"
                     << this->prevValue;
            qputenv(this->key, this->value.toLocal8Bit());
        }
    }

    TempEnv(const TempEnv &) = delete;
    TempEnv(TempEnv &&) = delete;
    TempEnv &operator=(const TempEnv &) = delete;
    TempEnv &operator=(TempEnv &&) = delete;

    ~TempEnv()
    {
        if (!prevValue.isNull())
        {
            qDebug() << "Reverting" << qPrintable(key) << "to"
                     << this->prevValue;
            qputenv(this->key, this->prevValue.toLocal8Bit());
        }
        else
        {
            qDebug() << "Unsetting" << qPrintable(key) << "to"
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

    auto config = getXDGDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(config.at(0), combinePath(home.getValue(), ".config/"));
    ASSERT_EQ(config.at(1), "/etc/xdg");

    ASSERT_EQ(config.length(), 2);
}

TEST(XDGDirectory, ConfigCustom)
{
    QTemporaryDir tmp;
    auto lock = environmentLock();

    TempEnv dataHome("XDG_CONFIG_HOME", "/tmp/home-data");
    TempEnv dataDirs("XDG_CONFIG_DIRS", "/tmp/sys-data-1:/tmp/sys-data-2");

    auto config = getXDGDirectories(XDGDirectoryType::Config);

    ASSERT_EQ(config.at(0), "/tmp/home-data");
    ASSERT_EQ(config.at(1), "/tmp/sys-data-1");
    ASSERT_EQ(config.at(2), "/tmp/sys-data-2");

    ASSERT_EQ(config.length(), 3);
}

/// Test the returned directories from XDGDirectoryType::Data when no extra environment variables are set
TEST(XDGDirectory, DataDefault)
{
    QTemporaryDir tmp;
    auto lock = environmentLock();

    TempEnv home("HOME", combinePath(tmp.path(), "home"));
    TempEnv dataHome("XDG_DATA_HOME", {});
    TempEnv dataDirs("XDG_DATA_DIRS", {});

    auto config = getXDGDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(config.at(0), combinePath(home.getValue(), ".local/share/"));
    ASSERT_EQ(config.at(1), "/usr/local/share/");
    ASSERT_EQ(config.at(2), "/usr/share/");

    ASSERT_EQ(config.length(), 3);
}

/// Test the returned directories from XDGDirectoryType::Data when user custom-configured environment variables are set
TEST(XDGDirectory, DataCustom)
{
    auto lock = environmentLock();

    TempEnv dataHome("XDG_DATA_HOME", "/tmp/home-share");
    TempEnv dataDirs("XDG_DATA_DIRS",
                     "/tmp/sys-share-1:/tmp/sys-share-2:/tmp/sys-share-3");

    auto config = getXDGDirectories(XDGDirectoryType::Data);

    ASSERT_EQ(config.at(0), "/tmp/home-share");
    ASSERT_EQ(config.at(1), "/tmp/sys-share-1");
    ASSERT_EQ(config.at(2), "/tmp/sys-share-2");
    ASSERT_EQ(config.at(3), "/tmp/sys-share-3");

    ASSERT_EQ(config.length(), 4);
}

#endif
