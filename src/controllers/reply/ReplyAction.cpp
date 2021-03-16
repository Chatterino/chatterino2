#include "ReplyAction.hpp"

#include <QRegularExpression>
#include "Application.hpp"
#include "messages/Image.hpp"
#include "singletons/Resources.hpp"

namespace chatterino {

ReplyAction::ReplyAction()
{
    this->imageToLoad_ = 3;
}

const boost::optional<ImagePtr> &ReplyAction::getImage() const
{
    assertInGuiThread();

    this->image_ = Image::fromPixmap(getResources().buttons.reply);

    return this->image_;
}

}  // namespace chatterino
