#pragma once

#include "controllers/hotkeys/GlobalShortcutFwd.hpp"

#include <QObject>

#include <memory>
#include <type_traits>

class QKeySequence;

namespace chatterino {

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

class GlobalShortcutPrivate;

/// This implementation is an adapted version of the QxtGlobalShortcut (from libqxt):
/// https://bitbucket.org/libqxt/libqxt/src/08d08b58c362000798e19c3b5979ad6a1e6e880a/src/widgets/qxtglobalshortcut.h
///
/// Backends are found in `platform/`.
class GlobalShortcut : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(QKeySequence shortcut READ shortcut WRITE setShortcut)

public:
    explicit GlobalShortcut(QObject *parent = nullptr);
    explicit GlobalShortcut(const QKeySequence &shortcut,
                            QObject *parent = nullptr);
    ~GlobalShortcut() override;

    QKeySequence shortcut() const;
    bool setShortcut(const QKeySequence &shortcut);

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enabled = true);
    void setDisabled(bool disabled = true);

Q_SIGNALS:
    void activated();

private:
    std::unique_ptr<GlobalShortcutPrivate> private_;
};

#endif

}  // namespace chatterino
