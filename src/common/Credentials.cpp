#include "Credentials.hpp"

#include "debug/AssertInGuiThread.hpp"
#include "keychain.h"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/Overloaded.hpp"

#include <QSaveFile>
#include <boost/variant.hpp>

#define FORMAT_NAME                                                  \
    ([&] {                                                           \
        assert(!provider.contains(":"));                             \
        return QString("chatterino:%1:%2").arg(provider).arg(name_); \
    })()

namespace chatterino {

namespace {
    bool useKeyring()
    {
        if (getPaths()->isPortable())
        {
            return false;
        }
        else
        {
#ifdef Q_OS_LINUX
            return getSettings()->useKeyring;
#else
            return true;
#endif
        }
    }

    // Insecure storage:
    QString insecurePath()
    {
        return combinePath(getPaths()->settingsDirectory, "credentials.json");
    }

    QJsonDocument loadInsecure()
    {
        QFile file(insecurePath());
        file.open(QIODevice::ReadOnly);
        return QJsonDocument::fromJson(file.readAll());
    }

    void storeInsecure(const QJsonDocument &doc)
    {
        QSaveFile file(insecurePath());
        file.open(QIODevice::WriteOnly);
        file.write(doc.toJson());
        file.commit();
    }

    QJsonDocument &insecureInstance()
    {
        static auto store = loadInsecure();
        return store;
    }

    void queueInsecureSave()
    {
        static bool isQueued = false;

        if (!isQueued)
        {
            isQueued = true;
            QTimer::singleShot(200, qApp, [] {
                storeInsecure(insecureInstance());
                isQueued = false;
            });
        }
    }

    // QKeychain runs jobs asyncronously, so we have to assure that set/erase
    // jobs gets executed in order.
    struct SetJob {
        QString name;
        QString credential;
    };

    struct EraseJob {
        QString name;
    };

    using Job = boost::variant<SetJob, EraseJob>;

    static std::queue<Job> &jobQueue()
    {
        static std::queue<Job> jobs;
        return jobs;
    }

    static void runNextJob()
    {
        auto &&queue = jobQueue();

        if (!queue.empty())
        {
            // we were gonna use std::visit here but macos is shit

            auto &&item = queue.front();

            if (item.which() == 0)  // set job
            {
                auto set = boost::get<SetJob>(item);
                auto job = new QKeychain::WritePasswordJob("chatterino");
                job->setAutoDelete(true);
                job->setKey(set.name);
                job->setTextData(set.credential);
                QObject::connect(job, &QKeychain::Job::finished, qApp,
                                 [](auto) { runNextJob(); });
                job->start();
            }
            else  // erase job
            {
                auto erase = boost::get<EraseJob>(item);
                auto job = new QKeychain::DeletePasswordJob("chatterino");
                job->setAutoDelete(true);
                job->setKey(erase.name);
                QObject::connect(job, &QKeychain::Job::finished, qApp,
                                 [](auto) { runNextJob(); });
                job->start();
            }

            queue.pop();
        }
    }

    static void queueJob(Job &&job)
    {
        auto &&queue = jobQueue();

        queue.push(std::move(job));
        if (queue.size() == 1)
        {
            runNextJob();
        }
    }
}  // namespace

Credentials &Credentials::instance()
{
    static Credentials creds;
    return creds;
}

Credentials::Credentials()
{
}

void Credentials::get(const QString &provider, const QString &name_,
                      QObject *receiver,
                      std::function<void(const QString &)> &&onLoaded)
{
    assertInGuiThread();

    auto name = FORMAT_NAME;

    if (useKeyring())
    {
        auto job = new QKeychain::ReadPasswordJob("chatterino");
        job->setAutoDelete(true);
        job->setKey(name);
        QObject::connect(
            job, &QKeychain::Job::finished, receiver,
            [job, onLoaded = std::move(onLoaded)](auto) mutable {
                onLoaded(job->textData());
            },
            Qt::DirectConnection);
        job->start();
    }
    else
    {
        auto &instance = insecureInstance();

        onLoaded(instance.object().find(name).value().toString());
    }
}

void Credentials::set(const QString &provider, const QString &name_,
                      const QString &credential)
{
    assertInGuiThread();

    /// On linux, we try to use a keychain but show a message to disable it when it fails.
    /// XXX: add said message

    auto name = FORMAT_NAME;

    if (useKeyring())
    {
        queueJob(SetJob{name, credential});
    }
    else
    {
        auto &instance = insecureInstance();

        instance.object()[name] = credential;

        queueInsecureSave();
    }
}

void Credentials::erase(const QString &provider, const QString &name_)
{
    assertInGuiThread();

    auto name = FORMAT_NAME;

    if (useKeyring())
    {
        queueJob(EraseJob{name});
    }
    else
    {
        auto &instance = insecureInstance();

        if (auto it = instance.object().find(name);
            it != instance.object().end())
        {
            instance.object().erase(it);
        }

        queueInsecureSave();
    }
}

}  // namespace chatterino
