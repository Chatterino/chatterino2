#include "ModerationAction.hpp"

#include <QRegularExpression>
#include "Application.hpp"
#include "messages/Image.hpp"
#include "singletons/Resources.hpp"

namespace chatterino {

// ModerationAction::ModerationAction(Image *_image, const QString &_action)
//    : _isImage(true)
//    , image(_image)
//    , action(_action)
//{
//}

// ModerationAction::ModerationAction(const QString &_line1, const QString
// &_line2,
//                                   const QString &_action)
//    : _isImage(false)
//    , image(nullptr)
//    , line1(_line1)
//    , line2(_line2)
//    , action(_action)
//{
//}

ModerationAction::ModerationAction(const QString &action)
    : action_(action)
{
    static QRegularExpression replaceRegex("[!/.]");
    static QRegularExpression timeoutRegex("^[./]timeout.* (\\d+)");

    auto timeoutMatch = timeoutRegex.match(action);

    if (timeoutMatch.hasMatch())
    {
        // if (multipleTimeouts > 1) {
        // QString line1;
        // QString line2;

        int amount = timeoutMatch.captured(1).toInt();

        if (amount < 60)
        {
            this->line1_ = QString::number(amount);
            this->line2_ = "s";
        }
        else if (amount < 60 * 60)
        {
            this->line1_ = QString::number(amount / 60);
            this->line2_ = "m";
        }
        else if (amount < 60 * 60 * 24)
        {
            this->line1_ = QString::number(amount / 60 / 60);
            this->line2_ = "h";
        }
        else
        {
            this->line1_ = QString::number(amount / 60 / 60 / 24);
            this->line2_ = "d";
        }

        // line1 = this->line1_;
        // line2 = this->line2_;
        // } else {
        //     this->_moderationActions.emplace_back(getResources().buttonTimeout,
        //     str);
        // }
    }
    else if (action.startsWith("/ban "))
    {
        this->imageToLoad_ = 1;
    }
    else if (action.startsWith("/delete "))
    {
        this->imageToLoad_ = 2;
    }
    else
    {
        QString xD = action;

        xD.replace(replaceRegex, "");

        this->line1_ = xD.mid(0, 2);
        this->line2_ = xD.mid(2, 2);
    }
}

bool ModerationAction::operator==(const ModerationAction &other) const
{
    return this == std::addressof(other);
}

bool ModerationAction::isImage() const
{
    return bool(this->image_);
}

const boost::optional<ImagePtr> &ModerationAction::getImage() const
{
    assertInGuiThread();

    if (this->imageToLoad_ != 0)
    {
        if (this->imageToLoad_ == 1)
            this->image_ = Image::fromPixmap(getResources().buttons.ban);
        else if (this->imageToLoad_ == 2)
            this->image_ = Image::fromPixmap(getResources().buttons.trashCan);
    }

    return this->image_;
}

const QString &ModerationAction::getLine1() const
{
    return this->line1_;
}

const QString &ModerationAction::getLine2() const
{
    return this->line2_;
}

const QString &ModerationAction::getAction() const
{
    return this->action_;
}

}  // namespace chatterino
