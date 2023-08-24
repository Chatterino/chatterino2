#include "controllers/hotkeys/GlobalShortcut.hpp"

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

#    include "common/Literals.hpp"
#    include "common/QLogging.hpp"
#    include "platform/GlobalShortcutPrivate.hpp"

#    include <QAbstractEventDispatcher>
#    include <QKeySequence>
#    include <QtDebug>

namespace chatterino {

using namespace literals;

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
#    ifndef Q_OS_MAC
size_t GlobalShortcutPrivate::REFCOUNT = 0;
#    endif  // Q_OS_MAC

decltype(GlobalShortcutPrivate::SHORTCUTS) GlobalShortcutPrivate::SHORTCUTS;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

GlobalShortcutPrivate::GlobalShortcutPrivate(GlobalShortcut *owner)
    : enabled(true)
    , key(Qt::Key(0))
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
    if (shortcut.count() > 1)
    {
        qCWarning(chatterinoHotkeys)
            << u"Global shortcuts must be composed of exactly one key with optional modifiers."_s;
    }

    auto allMods = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier |
                   Qt::MetaModifier;
    this->key = shortcut.isEmpty()
                    ? Qt::Key(0)
                    : Qt::Key((shortcut[0] ^ allMods) & shortcut[0]);
    this->mods = shortcut.isEmpty()
                     ? Qt::KeyboardModifiers(0)
                     : Qt::KeyboardModifiers(shortcut[0] & allMods);

    quint32 nativeKey = nativeKeycode(this->key);
    quint32 nativeMods = nativeModifiers(this->mods);
    auto res = registerShortcut(nativeKey, nativeMods);
    if (res.ok)
    {
        SHORTCUTS.emplace(std::make_pair(nativeKey, nativeMods), this->owner_);

        return true;
    }

    qCWarning(chatterinoHotkeys)
        << "GlobalShortcut failed to register:" << QKeySequence(key + mods)
        << "(native) error:" << res.error;

    return false;
}

bool GlobalShortcutPrivate::unsetShortcut()
{
    const quint32 nativeKey = nativeKeycode(key);
    const quint32 nativeMods = nativeModifiers(mods);
    auto it = SHORTCUTS.find(qMakePair(nativeKey, nativeMods));
    if (it != SHORTCUTS.end())
    {
        auto res = unregisterShortcut(nativeKey, nativeMods);
        if (res.ok)
        {
            SHORTCUTS.erase(it);
        }
        else
        {
            qWarning() << "GlobalShortcut failed to unregister:"
                       << QKeySequence(key + mods)
                       << "(native) error:" << res.error;
        }
        key = Qt::Key(0);
        mods = Qt::KeyboardModifiers(0);
        return res.ok;
    }

    return false;
}

void GlobalShortcutPrivate::activateShortcut(quint32 nativeKey,
                                             quint32 nativeMods)
{
    auto it = SHORTCUTS.find(std::make_pair(nativeKey, nativeMods));
    if (it != SHORTCUTS.end() && it->second->isEnabled())
    {
        emit it->second->activated();
    }
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

bool GlobalShortcut::isEnabled() const
{
    return this->private_->enabled;
}

void GlobalShortcut::setEnabled(bool enabled)
{
    this->private_->enabled = enabled;
}

void GlobalShortcut::setDisabled(bool disabled)
{
    this->private_->enabled = !disabled;
}

}  // namespace chatterino

#endif
