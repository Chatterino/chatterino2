#include "controllers/hotkeys/GlobalShortcut.hpp"

#ifdef CHATTERINO_HAS_GLOBAL_SHORTCUT

#    include "controllers/hotkeys/GlobalShortcutPrivate.hpp"

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
