#include "controllers/hotkeys/GlobalShortcut.hpp"

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

#    include "controllers/hotkeys/GlobalShortcutPrivate.hpp"
#    include "debug/AssertInGuiThread.hpp"

namespace chatterino {

GlobalShortcut::GlobalShortcut(QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<GlobalShortcutPrivate>(this))
{
}

GlobalShortcut::GlobalShortcut(const QKeySequence &shortcut, QObject *parent)
    : QObject(parent)
    , private_(std::make_unique<GlobalShortcutPrivate>(this))
{
    assertInGuiThread();
    this->setShortcut(shortcut);
}

GlobalShortcut::~GlobalShortcut()
{
    assertInGuiThread();
    this->unsetShortcut();
}

QKeySequence GlobalShortcut::shortcut() const noexcept
{
    assertInGuiThread();
    return {this->private_->key | this->private_->mods};
}

bool GlobalShortcut::setShortcut(const QKeySequence &shortcut)
{
    assertInGuiThread();
    if (this->private_->key != 0)
    {
        this->private_->unsetShortcut();
    }
    return this->private_->setShortcut(shortcut);
}

bool GlobalShortcut::unsetShortcut()
{
    assertInGuiThread();
    if (this->private_->key != 0)
    {
        return this->private_->unsetShortcut();
    }

    return true;
}

}  // namespace chatterino

#endif
