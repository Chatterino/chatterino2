#include "ModerationAction.hpp"

#include "singletons/ResourceManager.hpp"

namespace chatterino {

ModerationAction::ModerationAction(chatterino::Image *_image, const QString &_action)
    : _isImage(true)
    , image(_image)
    , action(_action)
{
}

ModerationAction::ModerationAction(const QString &_line1, const QString &_line2,
                                   const QString &_action)
    : _isImage(false)
    , image(nullptr)
    , line1(_line1)
    , line2(_line2)
    , action(_action)
{
}

bool ModerationAction::isImage() const
{
    return this->_isImage;
}

chatterino::Image *ModerationAction::getImage() const
{
    return this->image;
}

const QString &ModerationAction::getLine1() const
{
    return this->line1;
}

const QString &ModerationAction::getLine2() const
{
    return this->line2;
}

const QString &ModerationAction::getAction() const
{
    return this->action;
}

}  // namespace chatterino
