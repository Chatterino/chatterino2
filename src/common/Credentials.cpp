#include "Credentials.hpp"

#include "keychain.h"
#include "singletons/Paths.hpp"

#define FORMAT_NAME                                                  \
    ([&] {                                                           \
        assert(!provider.contains(":"));                             \
        return QString("chatterino:%1:%2").arg(provider).arg(name_); \
    })()

namespace chatterino {

Credentials &Credentials::getInstance()
{
    static Credentials creds;
    return creds;
}

Credentials::Credentials()
{
}

QString Credentials::get(const QString &provider, const QString &name_,
                         std::function<void(QString)> &&onLoaded)
{
    auto name = FORMAT_NAME;

    if (getPaths()->isPortable())
    {
        assert(false);

        return {};
    }
    else
    {
        auto job = new QKeychain::ReadPasswordJob("chatterino");
        job->setAutoDelete(true);
        job->setKey(name);
        QObject::connect(job, &QKeychain::Job::finished, qApp,
                         [job, onLoaded = std::move(onLoaded)](auto) mutable {
                             onLoaded(job->textData());
                         });
        job->start();

        return job->textData();
    }
}

void Credentials::set(const QString &provider, const QString &name_,
                      const QString &credential)
{
    auto name = FORMAT_NAME;

    if (getPaths()->isPortable())
    {
        assert(false);
    }
    else
    {
        auto job = new QKeychain::WritePasswordJob("chatterino");
        job->setAutoDelete(true);
        job->setKey(name);
        job->setTextData(credential);
        QObject::connect(job, &QKeychain::Job::finished, qApp, [](auto) {});
        job->start();
    }
}

void Credentials::erase(const QString &provider, const QString &name_)
{
    auto name = FORMAT_NAME;

    if (getPaths()->isPortable())
    {
        assert(false);
    }
    else
    {
        auto job = new QKeychain::DeletePasswordJob("chatterino");
        job->setAutoDelete(true);
        job->setKey(name);
        QObject::connect(job, &QKeychain::Job::finished, qApp, [](auto) {});
        job->start();
    }
}

}  // namespace chatterino
