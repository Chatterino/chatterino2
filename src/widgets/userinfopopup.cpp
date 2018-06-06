#include "userinfopopup.hpp"

#include "application.hpp"
#include "singletons/resourcemanager.hpp"
#include "util/layoutcreator.hpp"
#include "util/posttothread.hpp"
#include "util/urlfetch.hpp"
#include "widgets/helper/line.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"

#include <QCheckBox>
#include <QDesktopServices>
#include <QLabel>

namespace chatterino {
namespace widgets {

UserInfoPopup::UserInfoPopup()
    : BaseWindow(nullptr, BaseWindow::Flags(BaseWindow::Frameless | BaseWindow::DeleteOnFocusOut))
{
    auto app = getApp();

    auto layout = util::LayoutCreator<UserInfoPopup>(this).setLayoutType<QVBoxLayout>();

    // first line
    auto head = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        // avatar
        //        auto avatar = head.emplace<QLabel>("Avatar").assign(&this->ui_.avatarButtoAn);
        auto avatar = head.emplace<RippleEffectButton>(nullptr).assign(&this->ui_.avatarButton);
        avatar->setFixedSize(100, 100);
        QObject::connect(*avatar, &RippleEffectButton::clicked, [this] {
            QDesktopServices::openUrl(QUrl("https://twitch.tv/" + this->userName_));
        });

        // items on the right
        auto vbox = head.emplace<QVBoxLayout>();
        {
            auto name = vbox.emplace<QLabel>().assign(&this->ui_.nameLabel);

            auto font = name->font();
            font.setBold(true);
            name->setFont(font);
            vbox.emplace<QLabel>("Loading...").assign(&this->ui_.viewCountLabel);
            vbox.emplace<QLabel>().assign(&this->ui_.followerCountLabel);
            vbox.emplace<QLabel>().assign(&this->ui_.createdDateLabel);
        }
    }

    layout.emplace<Line>(false);

    // second line
    auto user = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        user->addStretch(1);

        auto ignore = user.emplace<QCheckBox>("Ignore").assign(&this->ui_.ignore);
        //        ignore->setEnabled(false);

        auto ignoreHighlights =
            user.emplace<QCheckBox>("Ignore highlights").assign(&this->ui_.ignoreHighlights);
        //        ignoreHighlights->setEnabled(false);

        auto mod = user.emplace<RippleEffectButton>(this);
        mod->setPixmap(app->resources->buttons.mod);
        mod->setScaleIndependantSize(30, 30);
        auto unmod = user.emplace<RippleEffectLabel>();
        unmod->setPixmap(app->resources->buttons.unmod);
        unmod->setScaleIndependantSize(30, 30);

        user->addStretch(1);
    }

    layout.emplace<Line>(false);

    // third line
    auto moderation = layout.emplace<QHBoxLayout>().withoutMargin();
    {
        moderation.emplace<TimeoutWidget>();
    }

    this->setStyleSheet("font-size: 11pt;");
}

void UserInfoPopup::setData(const QString &name, const ChannelPtr &channel)
{
    this->userName_ = name;
    this->channel_ = channel;

    this->ui_.nameLabel->setText(name);

    this->updateUserData();
}

void UserInfoPopup::updateUserData()
{
    util::twitch::get("https://api.twitch.tv/kraken/channels/" + this->userName_, this,
                      [=](const QJsonObject &obj) {
                          this->ui_.followerCountLabel->setText(
                              "Followers: " + QString::number(obj.value("followers").toInt()));
                          this->ui_.viewCountLabel->setText(
                              "Views: " + QString::number(obj.value("views").toInt()));
                          this->ui_.createdDateLabel->setText(
                              "Created: " + obj.value("created_at").toString().section("T", 0, 0));

                          this->loadAvatar(QUrl(obj.value("logo").toString()));
                      });
}

void UserInfoPopup::loadAvatar(const QUrl &url)
{
    QNetworkRequest req(url);
    static auto manager = new QNetworkAccessManager();
    auto *reply = manager->get(req);

    QObject::connect(reply, &QNetworkReply::finished, this, [=] {
        if (reply->error() == QNetworkReply::NoError) {
            const auto data = reply->readAll();

            // might want to cache the avatar image
            QPixmap avatar;
            avatar.loadFromData(data);
            this->ui_.avatarButton->setPixmap(avatar);
        } else {
            this->ui_.avatarButton->setPixmap(QPixmap());
        }
    });
}

//
// TimeoutWidget
//
UserInfoPopup::TimeoutWidget::TimeoutWidget()
    : BaseWidget(nullptr)
{
    auto layout =
        util::LayoutCreator<TimeoutWidget>(this).setLayoutType<QHBoxLayout>().withoutMargin();

    QColor color1(255, 255, 255, 80);
    QColor color2(255, 255, 255, 0);

    int buttonWidth = 40;
    int buttonWidth2 = 24;
    int buttonHeight = 32;

    layout->setSpacing(16);

    {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            title.emplace<QLabel>("unban");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto unban = hbox.emplace<RippleEffectButton>(nullptr);
                unban->setPixmap(getApp()->resources->buttons.unban);
                unban->setScaleIndependantSize(buttonHeight, buttonHeight);
                unban->setBorderColor(QColor(255, 255, 255, 127));
            }
        }
    }

    {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            title.emplace<QLabel>("sec");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("1");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
        }
    }

    {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            title.emplace<QLabel>("min");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("1");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("5");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("10");
                a->setScaleIndependantSize(buttonWidth, buttonHeight);
                a->setBorderColor(color1);
            }
        }
    }

    {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            title.emplace<QLabel>("hour");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("1");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("4");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
        }
    }

    {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            title.emplace<QLabel>("week");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("1");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
            {
                auto a = hbox.emplace<RippleEffectLabel>();
                a->getLabel().setText("2");
                a->setScaleIndependantSize(buttonWidth2, buttonHeight);
                a->setBorderColor(color1);
            }
        }
    }

    {
        auto vbox = layout.emplace<QVBoxLayout>().withoutMargin();
        {
            auto title = vbox.emplace<QHBoxLayout>().withoutMargin();
            title->addStretch(1);
            title.emplace<QLabel>("ban");
            title->addStretch(1);

            auto hbox = vbox.emplace<QHBoxLayout>().withoutMargin();
            hbox->setSpacing(0);
            {
                auto ban = hbox.emplace<RippleEffectButton>(nullptr);
                ban->setPixmap(getApp()->resources->buttons.ban);
                ban->setScaleIndependantSize(buttonHeight, buttonHeight);
                ban->setBorderColor(QColor(255, 255, 255, 127));
            }
        }
    }
}

void UserInfoPopup::TimeoutWidget::paintEvent(QPaintEvent *)
{
    //    QPainter painter(this);

    //    painter.setPen(QColor(255, 255, 255, 63));

    //    painter.drawLine(0, this->height() / 2, this->width(), this->height() / 2);
}

}  // namespace widgets
}  // namespace chatterino
