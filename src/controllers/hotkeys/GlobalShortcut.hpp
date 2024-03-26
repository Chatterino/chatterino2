#pragma once

#include "controllers/hotkeys/GlobalShortcutFwd.hpp"

#include <QKeySequence>
#include <QObject>

#include <memory>

namespace chatterino {

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

class GlobalShortcutPrivate;

/// @brief This class is used to create global keyboard shortcuts
///
/// Multiple instances can listen to the same shortcut/key combination. The
/// implementation takes care of de-duplication. All listeners will get the
/// event. Through the `consumerID` (n-th listener) and `singleConsumer`,
/// listeners can decide whether to accept the signal.
///
/// This class can ony be used from the GUI thread.
///
/// This implementation is an adapted version of the QxtGlobalShortcut (from libqxt):
/// https://bitbucket.org/libqxt/libqxt/src/08d08b58c362000798e19c3b5979ad6a1e6e880a/src/widgets/qxtglobalshortcut.h
///
/// Backends are found in `platform/`.
class GlobalShortcut : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)

public:
    explicit GlobalShortcut(QObject *parent = nullptr);
    explicit GlobalShortcut(const QKeySequence &shortcut,
                            QObject *parent = nullptr);
    ~GlobalShortcut() override;
    GlobalShortcut(const GlobalShortcut &) = delete;
    GlobalShortcut(GlobalShortcut &&) = delete;
    GlobalShortcut &operator=(const GlobalShortcut &) = delete;
    GlobalShortcut &operator=(GlobalShortcut &&) = delete;

    /// Returns the current key combination
    [[nodiscard]] QKeySequence shortcut() const noexcept;

    /// @brief Connects the listener to the @a shortcut
    ///
    /// If there isn't any global shortcut registered on the platform, this
    /// will register one.
    ///
    /// @returns @c true if the listener was successfully connected
    bool setShortcut(const QKeySequence &shortcut);

    /// @brief Disconnects this listener from the shortcut
    ///
    /// If this listener is the only one, the shortcut will be unregistered on
    /// the platform.
    ///
    /// @returns @c true if the listener was disconnected
    bool unsetShortcut();

Q_SIGNALS:
    void activated(size_t consumerID, bool singleConsumer);

private:
    std::unique_ptr<GlobalShortcutPrivate> private_;
};

#endif

}  // namespace chatterino
