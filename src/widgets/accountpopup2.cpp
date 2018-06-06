#include "accountpopup2.hpp"

#include "application.hpp"
#include "singletons/resourcemanager.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/helper/rippleeffectlabel.hpp"

#include <QLabel>

namespace chatterino {
namespace widgets {

AccountPopup2::AccountPopup2()
    : BaseWindow(nullptr, BaseWindow::Frameless)
{
    auto app = getApp();

    auto layout = util::LayoutCreator<AccountPopup2>(this).setLayoutType<QVBoxLayout>();

    // first line
    auto head = layout.emplace<QHBoxLayout>();
    {
        // avatar
        auto avatar = head.emplace<QLabel>("Avatar").assign(&this->ui.label);
        avatar->setFixedSize(100, 100);

        // items on the right
        auto vbox = head.emplace<QVBoxLayout>();
        {
            vbox.emplace<QLabel>("Name");
            vbox.emplace<QLabel>("Views");
            vbox.emplace<QLabel>("Followers");
            vbox.emplace<QLabel>("Create date");
        }
    }

    // second line
    auto user = layout.emplace<QHBoxLayout>();
    {
        user->addStretch(1);

        auto ignore = user.emplace<RippleEffectLabel>();
        ignore->getLabel().setText("Ignore");
        ignore->setScaleIndependantHeight(24);

        auto ignoreHighlights = user.emplace<RippleEffectLabel>();
        ignoreHighlights->getLabel().setText("Ignore highlights");
        ignoreHighlights->setScaleIndependantHeight(24);

        user->addStretch(1);
    }

    // third line
    auto moderation = layout.emplace<QHBoxLayout>();
    moderation->setSpacing(0);
    {
        auto unban = moderation.emplace<RippleEffectLabel>();
        unban->setPixmap(app->resources->buttons.unban);
        unban->setScaleIndependantSize(32, 32);
        unban->setBorderColor(QColor(255, 255, 255, 127));

        moderation.emplace<TimeoutWidget>();

        auto ban = moderation.emplace<RippleEffectLabel>();
        ban->setPixmap(app->resources->buttons.ban);
        ban->setScaleIndependantSize(32, 32);
        ban->setBorderColor(QColor(255, 255, 255, 127));

        // auto mod = moderation.emplace<RippleEffectButton>(this);
        // mod->setPixmap(app->resources->buttons.mod);
        // mod->setScaleIndependantSize(30, 30);
        // auto unmod = moderation.emplace<RippleEffectLabel>();
        // unmod->setPixmap(app->resources->buttons.unmod);
        // unmod->setScaleIndependantSize(30, 30);

        this->userStateChanged.connect([=]() mutable {
            // mod->setVisible(this->isBroadcaster_);
            // unmod->setVisible(this->isBroadcaster_);

            ban->setVisible(this->isMod_);
            unban->setVisible(this->isMod_);
        });
    }

    this->setStyleSheet("font-size: 12pt");
}

// void AccountPopup2::scaleChangedEvent(float newScale)
//{
//}

void AccountPopup2::setName(const QString &name)
{
}

void AccountPopup2::setChannel(const ChannelPtr &_channel)
{
}

//
// TimeoutWidget
//
AccountPopup2::TimeoutWidget::TimeoutWidget()
    : BaseWidget(nullptr)
{
    auto layout = util::LayoutCreator<TimeoutWidget>(this).setLayoutType<QHBoxLayout>();

    QColor color1(255, 255, 255, 127);
    QColor color2(255, 255, 255, 80);

    int buttonWidth = 40;
    int buttonWidth2 = 20;
    int buttonHeight = 32;

    layout->setSpacing(1);

    auto a = layout.emplace<RippleEffectLabel>();
    a->getLabel().setText("1s");
    a->setScaleIndependantSize(buttonWidth, buttonHeight);
    a->setBorderColor(color1);
    auto a1 = layout.emplace<RippleEffectLabel>(nullptr);
    //    a1->getLabel().setText("1m");
    a1->setScaleIndependantSize(buttonWidth2, buttonHeight);
    a1->setBorderColor(color2);
    auto a2 = layout.emplace<RippleEffectLabel>(nullptr);
    //    a2->getLabel().setText("5m");
    a2->setScaleIndependantSize(buttonWidth2, buttonHeight);
    a2->setBorderColor(color2);

    auto b = layout.emplace<RippleEffectLabel>();
    b->getLabel().setText("10m");
    b->setScaleIndependantSize(buttonWidth, buttonHeight);
    b->setBorderColor(color1);
    auto b1 = layout.emplace<RippleEffectLabel>(nullptr);
    b1->setScaleIndependantSize(buttonWidth2, buttonHeight);
    b1->setBorderColor(color2);
    //    b1->getLabel().setText("1h");
    auto b2 = layout.emplace<RippleEffectLabel>(nullptr);
    b2->setScaleIndependantSize(buttonWidth2, buttonHeight);
    b2->setBorderColor(color2);
    //    b2->getLabel().setText("4h");

    auto c = layout.emplace<RippleEffectLabel>();
    c->getLabel().setText("1d");
    c->setScaleIndependantSize(buttonWidth, buttonHeight);
    c->setBorderColor(color1);
    auto c1 = layout.emplace<RippleEffectLabel>(nullptr);
    //    c1->getLabel().setText("3d");
    c1->setScaleIndependantSize(buttonWidth2, buttonHeight);
    c1->setBorderColor(color2);
    auto c2 = layout.emplace<RippleEffectLabel>(nullptr);
    //    c2->getLabel().setText("1w");
    c2->setScaleIndependantSize(buttonWidth2, buttonHeight);
    c2->setBorderColor(color2);

    auto d = layout.emplace<RippleEffectLabel>();
    d->getLabel().setText("2w");
    d->setScaleIndependantSize(buttonWidth, buttonHeight);
    d->setBorderColor(color1);
}

void AccountPopup2::TimeoutWidget::paintEvent(QPaintEvent *)
{
    //    QPainter painter(this);

    //    painter.setPen(QColor(255, 255, 255, 63));

    //    painter.drawLine(0, this->height() / 2, this->width(), this->height() / 2);
}

}  // namespace widgets
}  // namespace chatterino
