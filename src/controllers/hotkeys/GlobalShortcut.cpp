#include "controllers/hotkeys/GlobalShortcut.hpp"

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

#    include "common/QLogging.hpp"
#    include "platform/GlobalShortcutPrivate.hpp"

#    include <QAbstractEventDispatcher>
#    include <QKeySequence>
#    include <QtDebug>

namespace chatterino {

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
#    ifndef Q_OS_MAC
size_t GlobalShortcutPrivate::REFCOUNT = 0;
#    endif  // Q_OS_MAC

decltype(GlobalShortcutPrivate::SHORTCUTS) GlobalShortcutPrivate::SHORTCUTS;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

GlobalShortcutPrivate::GlobalShortcutPrivate(GlobalShortcut *owner)
    : key(Qt::Key(0))
    , mods(Qt::NoModifier)
    , owner_(owner)
{
#    ifndef Q_OS_MAC
    if (REFCOUNT == 0)
    {
        QAbstractEventDispatcher::instance()->installNativeEventFilter(this);
    }
    REFCOUNT++;
#    endif  // Q_OS_MAC
}

GlobalShortcutPrivate::~GlobalShortcutPrivate()
{
#    ifndef Q_OS_MAC
    REFCOUNT--;
    if (REFCOUNT == 0)
    {
        QAbstractEventDispatcher *dispatcher =
            QAbstractEventDispatcher::instance();
        if (dispatcher != nullptr)
        {
            dispatcher->removeNativeEventFilter(this);
        }
    }
#    endif  // Q_OS_MAC
}

bool GlobalShortcutPrivate::setShortcut(const QKeySequence &shortcut)
{
    // our caller already unset the shortcut

    if (shortcut.isEmpty())
    {
        // same as unsetShortcut
        return false;
    }

    if (shortcut.count() > 1)
    {
        qCWarning(chatterinoHotkeys)
            << "Global shortcuts must be composed of exactly one key with "
               "optional modifiers.";
        return false;
    }

#    if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    this->key = shortcut[0].key();
    this->mods = shortcut[0].keyboardModifiers();
#    else
    this->key = Qt::Key(shortcut[0] & ~Qt::KeyboardModifierMask);
    this->mods = Qt::KeyboardModifiers(shortcut[0] & Qt::KeyboardModifierMask);
#    endif

    auto native = this->native();
    auto it = SHORTCUTS.find(native);
    if (it != SHORTCUTS.end())
    {
        // tap into the curren shortcut
        it->second.emplace_back(this->owner_);
        return true;
    }

    // no previous shortcut registered
    auto res = registerShortcut(native);
    if (!res.ok)
    {
        qCWarning(chatterinoHotkeys) << "GlobalShortcut failed to register:"
                                     << QKeySequence(this->key | this->mods)
                                     << "(native) error:" << res.error;
        return false;
    }
    SHORTCUTS.emplace(native, std::vector<GlobalShortcut *>{this->owner_});

    return true;
}

bool GlobalShortcutPrivate::unsetShortcut()
{
    auto native = this->native();
    auto shortcut = SHORTCUTS.find(native);
    if (shortcut == SHORTCUTS.end())
    {
        return false;
    }

    auto it = std::find(shortcut->second.begin(), shortcut->second.end(),
                        this->owner_);
    if (it == shortcut->second.end())
    {
        return false;
    }

    shortcut->second.erase(it);
    if (!shortcut->second.empty())
    {
        return true;
    }

    // No remaining shortcut -> unregister
    auto res = GlobalShortcutPrivate::unregisterShortcut(native);
    if (res.ok)
    {
        SHORTCUTS.erase(shortcut);
    }
    else
    {
        qWarning() << "GlobalShortcut failed to unregister:"
                   << QKeySequence(this->key | this->mods)
                   << "(native) error:" << res.error;
    }
    this->key = Qt::Key(0);
    this->mods = Qt::NoModifier;
    return res.ok;
}

void GlobalShortcutPrivate::activateShortcut(Native native)
{
    auto it = SHORTCUTS.find(native);
    if (it != SHORTCUTS.end())
    {
        bool singleConsumer = it->second.size() == 1;
        for (size_t i = 0; auto *consumer : it->second)
        {
            emit consumer->activated(i, singleConsumer);
            i++;
        }
    }
}

GlobalShortcutPrivate::Native GlobalShortcutPrivate::native() const
{
    return {
        .key = GlobalShortcutPrivate::nativeKeycode(this->key),
        .modifiers = GlobalShortcutPrivate::nativeModifiers(this->mods),
    };
}

GlobalShortcut::GlobalShortcut(QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<GlobalShortcutPrivate>(this))
{
}

GlobalShortcut::GlobalShortcut(const QKeySequence &shortcut, QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<GlobalShortcutPrivate>(this))
{
    setShortcut(shortcut);
}

GlobalShortcut::~GlobalShortcut()
{
    if (this->private_->key != 0)
    {
        this->private_->unsetShortcut();
    }
}

QKeySequence GlobalShortcut::shortcut() const
{
    return {this->private_->key | this->private_->mods};
}

bool GlobalShortcut::setShortcut(const QKeySequence &shortcut)
{
    if (this->private_->key != 0)
    {
        this->private_->unsetShortcut();
    }
    return this->private_->setShortcut(shortcut);
}

}  // namespace chatterino

#endif
