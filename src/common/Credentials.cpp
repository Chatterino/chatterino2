#include "common/Credentials.hpp"

#include "Application.hpp"
#include "common/Modes.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/Variant.hpp"

#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStringBuilder>

#include <variant>

#ifndef NO_QTKEYCHAIN
#    ifdef CMAKE_BUILD
#        if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#            include "qt6keychain/keychain.h"
#        else
#            include "qt5keychain/keychain.h"
#        endif
#    else
#        include <qtkeychain/keychain.h>
#    endif
#endif

namespace {

using namespace chatterino;

QString formatName(const QString &provider, const QString &name)
{
    assert(!provider.contains(":"));
    return u"chatterino:" % provider % u':' % name;
}

bool useKeyring()
{
#ifdef NO_QTKEYCHAIN
    return false;
#endif
    if (Modes::instance().isPortable)
    {
        return false;
    }

#ifdef Q_OS_LINUX
    return getSettings()->useKeyring;
#else
    return true;
#endif
}

// Insecure storage:
QString insecurePath()
{
    return combinePath(getApp()->getPaths().settingsDirectory,
                       "credentials.json");
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
        QTimer::singleShot(200, QApplication::instance(), [] {
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

using Job = std::variant<SetJob, EraseJob>;

std::queue<Job> &jobQueue()
{
    static std::queue<Job> jobs;
    return jobs;
}

void runNextJob()
{
#ifndef NO_QTKEYCHAIN
    auto &&queue = jobQueue();

    if (!queue.empty())
    {
        // we were gonna use std::visit here but macos is shit

        auto &&item = queue.front();

        std::visit(
            variant::Overloaded{
                [](const SetJob &set) {
                    auto *job = new QKeychain::WritePasswordJob("chatterino");
                    job->setAutoDelete(true);
                    job->setKey(set.name);
                    job->setTextData(set.credential);
                    QObject::connect(job, &QKeychain::Job::finished,
                                     QApplication::instance(), [](auto) {
                                         runNextJob();
                                     });
                    job->start();
                },
                [](const EraseJob &erase) {
                    auto *job = new QKeychain::DeletePasswordJob("chatterino");
                    job->setAutoDelete(true);
                    job->setKey(erase.name);
                    QObject::connect(job, &QKeychain::Job::finished,
                                     QApplication::instance(), [](auto) {
                                         runNextJob();
                                     });
                    job->start();
                },
            },
            item);

        queue.pop();
    }
#endif
}

void queueJob(Job &&job)
{
    auto &&queue = jobQueue();

    queue.push(std::move(job));
    if (queue.size() == 1)
    {
        runNextJob();
    }
}

}  // namespace

namespace chatterino {

Credentials &Credentials::instance()
{
    static Credentials creds;
    return creds;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Credentials::get(const QString &provider, const QString &name_,
                      QObject *receiver,
                      std::function<void(const QString &)> &&onLoaded)
{
    assertInGuiThread();

    auto name = formatName(provider, name_);

    if (useKeyring())
    {
#ifndef NO_QTKEYCHAIN
        // if NO_QTKEYCHAIN is set, then this code is never used either way
        auto *job = new QKeychain::ReadPasswordJob("chatterino");
        job->setAutoDelete(true);
        job->setKey(name);
        QObject::connect(
            job, &QKeychain::Job::finished, receiver,
            [job, onLoaded = std::move(onLoaded)](auto) mutable {
                onLoaded(job->textData());
            },
            Qt::DirectConnection);
        job->start();
#endif
    }
    else
    {
        const auto &instance = insecureInstance();

        onLoaded(instance[name].toString());
    }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Credentials::set(const QString &provider, const QString &name_,
                      const QString &credential)
{
    assertInGuiThread();

    /// On linux, we try to use a keychain but show a message to disable it when it fails.
    /// XXX: add said message

    auto name = formatName(provider, name_);

    if (useKeyring())
    {
        queueJob(SetJob{name, credential});
    }
    else
    {
        auto &instance = insecureInstance();

        auto obj = instance.object();
        obj[name] = credential;
        instance.setObject(obj);

        queueInsecureSave();
    }
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void Credentials::erase(const QString &provider, const QString &name_)
{
    assertInGuiThread();

    auto name = formatName(provider, name_);

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
