#pragma once

#include <type_traits>
#ifndef Q_OS_MAC
#    include <QAbstractNativeEventFilter>
#endif

#include <boost/functional/hash.hpp>
#include <QKeySequence>

#include <unordered_map>
#include <utility>
#include <vector>

namespace chatterino {

class GlobalShortcut;

struct GlobalShortcutResult {
    bool ok;
    uint32_t error;
};

class GlobalShortcutPrivate
#ifndef Q_OS_MAC
    : public QAbstractNativeEventFilter
#endif
{
public:
    struct Native {
        quint32 key;
        quint32 modifiers;

        bool operator==(Native rhs) const noexcept
        {
            return this->key == rhs.key && this->modifiers == rhs.modifiers;
        }
    };

    GlobalShortcutPrivate(GlobalShortcut *owner);
    ~GlobalShortcutPrivate() override;

    Qt::Key key;
    Qt::KeyboardModifiers mods;

    bool setShortcut(const QKeySequence &shortcut);
    bool unsetShortcut();

#ifndef Q_OS_MAC
#    if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    using NativeEventResult = qintptr;
#    else
    using NativeEventResult = long;
#    endif
    bool nativeEventFilter(const QByteArray &eventType, void *message,
                           NativeEventResult *result) override;
#endif

    static void activateShortcut(Native native);

private:
    Native native() const;

    static quint32 nativeKeycode(Qt::Key keycode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    static GlobalShortcutResult registerShortcut(Native);
    static GlobalShortcutResult unregisterShortcut(Native);

    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    static std::unordered_map<Native, std::vector<GlobalShortcut *>> SHORTCUTS;
#ifndef Q_OS_MAC
    static size_t REFCOUNT;
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
