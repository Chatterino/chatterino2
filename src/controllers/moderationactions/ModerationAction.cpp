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
    static QRegularExpression timeoutRegex("^[./]timeout.* (\\d+)([mhdw]?)");

    auto timeoutMatch = timeoutRegex.match(action);

    if (timeoutMatch.hasMatch())
    {
        // if (multipleTimeouts > 1) {
        // QString line1;
        // QString line2;

        constexpr int minute = 60;
        constexpr int hour = 60 * minute;
        constexpr int day = 24 * hour;
        constexpr int week = 7 * day;

        int amount = timeoutMatch.captured(1).toInt();
        QString unit = timeoutMatch.captured(2);

        if (unit == "m")
        {
            amount *= minute;
        }
        else if (unit == "h")
        {
            amount *= hour;
        }
        else if (unit == "d")
        {
            amount *= day;
        }
        else if (unit == "w")
        {
            amount *= week;
        }

        if (amount < minute)
        {
            this->line1_ = QString::number(amount);
            this->line2_ = "s";
        }
        else if (amount < hour)
        {
            this->line1_ = QString::number(amount / minute);
            this->line2_ = "m";
        }
        else if (amount < day)
        {
            this->line1_ = QString::number(amount / hour);
            this->line2_ = "h";
        }
        else if (amount < week)
        {
            this->line1_ = QString::number(amount / day);
            this->line2_ = "d";
        }
        else
        {
            this->line1_ = QString::number(amount / week);
            this->line2_ = "w";
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
