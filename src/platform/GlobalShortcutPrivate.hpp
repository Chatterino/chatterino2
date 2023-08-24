#pragma once

#include <QKeySequence>

#include <type_traits>
#ifndef Q_OS_MAC
#    include <QAbstractNativeEventFilter>
#endif

#include <boost/functional/hash.hpp>

#include <unordered_map>
#include <utility>

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
    GlobalShortcutPrivate(GlobalShortcut *owner);
    ~GlobalShortcutPrivate() override;

    bool enabled;
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

    static void activateShortcut(quint32 nativeKey, quint32 nativeMods);

private:
    static quint32 nativeKeycode(Qt::Key keycode);
    static quint32 nativeModifiers(Qt::KeyboardModifiers modifiers);

    static GlobalShortcutResult registerShortcut(quint32 nativeKey,
                                                 quint32 nativeMods);
    static GlobalShortcutResult unregisterShortcut(quint32 nativeKey,
                                                   quint32 nativeMods);

    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
    static std::unordered_map<std::pair<quint32, quint32>, GlobalShortcut *,
                              boost::hash<std::pair<quint32, quint32>>>
        SHORTCUTS;
#ifndef Q_OS_MAC
    static size_t REFCOUNT;
#endif
    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    GlobalShortcut *owner_ = nullptr;
};

}  // namespace chatterino
