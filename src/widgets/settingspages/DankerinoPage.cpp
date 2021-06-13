#include "DankerinoPage.hpp"

#include <QFontDialog>
#include <QLabel>
#include <QScrollArea>

#include "Application.hpp"
#include "common/Version.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/Line.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"

#include <QDesktopServices>
#include <QFileDialog>

namespace chatterino {

DankerinoPage::DankerinoPage()
{
    auto y = new QVBoxLayout;
    auto x = new QHBoxLayout;
    auto view = new GeneralPageView;
    this->view_ = view;
    x->addWidget(view);
    auto z = new QFrame;
    z->setLayout(x);
    y->addWidget(z);
    this->setLayout(y);

    this->initLayout(*view);
}

bool DankerinoPage::filterElements(const QString &query)
{
    if (this->view_)
        return this->view_->filterElements(query) || query.isEmpty();
    else
        return false;
}

void DankerinoPage::initLayout(GeneralPageView &layout)
{
    auto &s = *getSettings();

    layout.addTitle("Apperance");
    layout.addCheckbox("Show placeholder in text input box (requires restart)",
                       s.showTextInputPlaceholder);
    layout.addCheckbox("Colorize usernames on IRC", s.colorizeNicknamesOnIrc);
    layout.addTitle("Behavior");
    layout.addCheckbox("Lowercase tab-completed usernames",
                       s.lowercaseUsernames);
    layout.addTitle("Emotes");
    layout.addCheckbox("Enable loading 7TV emotes", s.enableLoadingSevenTV);
    layout.addTitle("Miscellaneous");
    layout.addIntInput("High rate limit spam delay in milliseconds (mod/vip)",
                       s.twitchHighRateLimitDelay, 100, 2000, 100);
    layout.addIntInput(
        "Low rate limit spam delay in milliseconds (non mod/vip)",
        s.twitchLowRateLimitDelay, 500, 3000, 1100);

    if (s.dankerinoThreeLetterApiEasterEgg)
    {
        layout.addCheckbox(
            "Click to disable GraphQL easter egg (requires restart)",
            s.dankerinoThreeLetterApiEasterEgg);
    }
    layout.addStretch();
    // invisible element for width
    auto inv = new BaseWidget(this);
    //    inv->setScaleIndependantWidth(600);
    layout.addWidget(inv);
}

}  // namespace chatterino
