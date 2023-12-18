#pragma once

#include "controllers/hotkeys/GlobalShortcutFwd.hpp"

#ifndef Q_OS_MAC
#    include <QAbstractNativeEventFilter>
#endif

#include <boost/functional/hash.hpp>
#include <QKeySequence>

#include <unordered_map>
#include <vector>

namespace chatterino {

class GlobalShortcut;

struct GlobalShortcutResult {
    bool ok;
    uint32_t error;
};

class GlobalShortcutPrivate
{
public:
    /// A native representation of a key and its modifiers.
    struct Native {
        quint32 key;
        quint32 modifiers;

        bool operator==(Native rhs) const noexcept
        {
            return this->key == rhs.key && this->modifiers == rhs.modifiers;
        }
    };

    GlobalShortcutPrivate(GlobalShortcut *owner);
    ~GlobalShortcutPrivate();
    GlobalShortcutPrivate(const GlobalShortcutPrivate &) = delete;
    GlobalShortcutPrivate(GlobalShortcutPrivate &&) = delete;
    GlobalShortcutPrivate &operator=(const GlobalShortcutPrivate &) = delete;
    GlobalShortcutPrivate &operator=(GlobalShortcutPrivate &&) = delete;

    Qt::Key key;
    Qt::KeyboardModifiers mods;

    /// Try to register the shortcut globally
    /// @returns true if the shortcut was registered
    bool setShortcut(const QKeySequence &shortcut);
    /// Try to unregister the shortcut globally
    /// @returns true if the shortcut was unregistered
    bool unsetShortcut();

    /// Handles activation of the spcified shortcut.
    static void activateShortcut(Native native);

private:
    /// @returns the native keycode of this shortcut
    Native native() const;

    /// Convert a Qt key to a native keycode.
    /// If no native keycode maps to the specified Qt key, `0` is returned.
    ///
    /// This is implemented per platform.
    static quint32 nativeKeycode(Qt::Key keycode);
    /// Convert a Qt modifier to a native modifier-code.
    /// If no native code maps to the specified Qt modifier, `0` is returned.
    ///
    /// This is implemented per platform.
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    /// Registers the shortcut on the platform.
    /// This is only called if this shortcut wasn't registered yet.
    ///
    /// This is implemented per platform.
    static GlobalShortcutResult registerShortcut(Native native);
    /// Unregisters the shortcut on the platform.
    /// This is only called if this shortcut was registered.
    ///
    /// This is implemented per platform.
    static GlobalShortcutResult unregisterShortcut(Native native);

    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)

    /// Global table with all registered shortcuts and all handlers for them.
    static std::unordered_map<Native, std::vector<GlobalShortcut *>> SHORTCUTS;

#ifndef Q_OS_MAC
    struct EventFilter : public QAbstractNativeEventFilter {
    public:
#    if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
        using NativeEventResult = qintptr;
#    else
        using NativeEventResult = long;
#    endif
        /// Filter for global hotkeys.
        ///
        /// This is implemented per platform.
        /// Implementations call #activateShortcut() when a shortcut is triggered.
        ///
        /// @sa #activateShortcut()
        bool nativeEventFilter(const QByteArray &eventType, void *message,
                               NativeEventResult *result) override;
    };

    /// Global reference-count of instances of this class.
    /// Used to manage the event-filter.
    static size_t REFCOUNT;
    /// The currently active event filter.
    /// There is at most one event filter active.
    /// Allocated with `new`.
    static EventFilter *CURRENT_FILTER;
#endif

    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    GlobalShortcut *owner_;
};

}  // namespace chatterino

template <>
struct std::hash<chatterino::GlobalShortcutPrivate::Native> {
    std::size_t operator()(
        chatterino::GlobalShortcutPrivate::Native const &n) const noexcept
    {
        std::size_t seed = 0;
        boost::hash_combine(seed, n.key);
        boost::hash_combine(seed, n.modifiers);

        return seed;
    }
};
