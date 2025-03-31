#include "providers/twitch/TwitchUsers.hpp"

#include "common/QLogging.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchUser.hpp"

#include <boost/unordered/unordered_flat_map.hpp>
#include <QStringList>
#include <QTimer>

namespace {

auto withSelf(auto *ptr, auto cb)
{
    return [weak{ptr->weak_from_this()}, cb = std::move(cb)](auto &&...args) {
        auto self = weak.lock();
        if (!self)
        {
            return;
        }
        cb(std::move(self), std::forward<decltype(args)>(args)...);
    };
}

}  // namespace

namespace chatterino {

class TwitchUsersPrivate
    : public std::enable_shared_from_this<TwitchUsersPrivate>
{
public:
    TwitchUsersPrivate();

private:
    boost::unordered_flat_map<UserId, std::shared_ptr<TwitchUser>> cache;
    QStringList unresolved;
    QTimer nextBatchTimer;
    bool isResolving = false;

    std::shared_ptr<TwitchUser> makeUnresolved(const UserId &id);
    void makeNextRequest();
    void updateUsers(const std::vector<HelixUser> &users);

    friend TwitchUsers;
};

TwitchUsers::TwitchUsers()
    : private_(new TwitchUsersPrivate)
{
}

TwitchUsers::~TwitchUsers() = default;

std::shared_ptr<TwitchUser> TwitchUsers::resolveID(const UserId &id)
{
    auto cached = this->private_->cache.find(id);
    if (cached != this->private_->cache.end())
    {
        return cached->second;
    }
    return this->private_->makeUnresolved(id);
}

TwitchUsersPrivate::TwitchUsersPrivate()
{
    this->nextBatchTimer.setSingleShot(true);
    // Wait for multiple request batches to come in before making a request
    this->nextBatchTimer.setInterval(250);

    QObject::connect(&this->nextBatchTimer, &QTimer::timeout, [this] {
        this->makeNextRequest();
    });
}

std::shared_ptr<TwitchUser> TwitchUsersPrivate::makeUnresolved(const UserId &id)
{
    // assumption: Cache entry is empty so neither a shared pointer was created
    //             nor an entry in the unresolved list was added.
    auto ptr = this->cache
                   .emplace(id, std::make_shared<TwitchUser>(TwitchUser{
                                    .id = id.string,
                                    .name = {},
                                    .displayName = {},
                                }))
                   .first->second;
    if (id.string.isEmpty())
    {
        return ptr;
    }

    this->unresolved.append(id.string);
    if (!this->isResolving && !this->nextBatchTimer.isActive())
    {
        this->nextBatchTimer.start();
    }
    return ptr;
}

void TwitchUsersPrivate::makeNextRequest()
{
    if (this->unresolved.empty())
    {
        return;
    }

    if (this->isResolving)
    {
        qCWarning(chatterinoTwitch) << "Tried to start request while resolving";
        return;
    }
    this->isResolving = true;

    auto ids = this->unresolved.mid(
        0, std::min<qsizetype>(this->unresolved.size(), 100));
    this->unresolved = this->unresolved.mid(ids.length());
    getHelix()->fetchUsers(ids, {},
                           withSelf(this,
                                    [](auto self, const auto &users) {
                                        self->updateUsers(users);
                                        self->isResolving = false;
                                        self->makeNextRequest();
                                    }),
                           withSelf(this, [](auto self) {
                               qCWarning(chatterinoTwitch)
                                   << "Failed to load users";
                               self->isResolving = false;
                               self->makeNextRequest();
                           }));
}

void TwitchUsersPrivate::updateUsers(const std::vector<HelixUser> &users)
{
    for (const auto &user : users)
    {
        auto cached = this->cache.find(UserId{user.id});
        if (cached == this->cache.end())
        {
            qCWarning(chatterinoTwitch) << "Couldn't find user" << user.login
                                        << "with id" << user.id << "in cache";
            continue;
        }
        cached->second->update(user);
    }
}

}  // namespace chatterino
