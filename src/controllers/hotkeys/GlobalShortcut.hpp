#pragma once

#include "controllers/hotkeys/GlobalShortcutFwd.hpp"

#include <QKeySequence>
#include <QObject>

#include <memory>
#include <type_traits>

namespace chatterino {

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

class GlobalShortcutPrivate;

/// This implementation is an adapted version of the QxtGlobalShortcut (from libqxt):
/// https://bitbucket.org/libqxt/libqxt/src/08d08b58c362000798e19c3b5979ad6a1e6e880a/src/widgets/qxtglobalshortcut.h
///
/// Backends are found in `platform/`.
///
/// In this implementation, multiple `GlobalShortcut`s can listen to the same key combination.
/// All listeners will get the event. Through the `consumerID` (n-th listener) and `singleConsumer`,
/// listeners can decide whether to accept the signal.
class GlobalShortcut : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)

public:
    explicit GlobalShortcut(QObject *parent = nullptr);
    explicit GlobalShortcut(const QKeySequence &shortcut,
                            QObject *parent = nullptr);
    ~GlobalShortcut() override;

    QKeySequence shortcut() const;
    bool setShortcut(const QKeySequence &shortcut);

Q_SIGNALS:
    void activated(size_t consumerID, bool singleConsumer);

private:
    std::unique_ptr<GlobalShortcutPrivate> private_;
};

#endif

}  // namespace chatterino
