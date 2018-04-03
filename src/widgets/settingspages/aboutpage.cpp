#include "aboutpage.hpp"

#include "util/layoutcreator.hpp"

#include <QLabel>
#include <QVBoxLayout>

#define PIXMAP_WIDTH 500

namespace chatterino {
namespace widgets {
namespace settingspages {

AboutPage::AboutPage()
    : SettingsPage("About", ":/images/about.svg")
{
    util::LayoutCreator<AboutPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        QPixmap pixmap;
        pixmap.load(":/images/aboutlogo.png");

        auto logo = layout.emplace<QLabel>().assign(&this->logo);
        logo->setPixmap(pixmap);
        logo->setFixedSize(PIXMAP_WIDTH, PIXMAP_WIDTH * pixmap.height() / pixmap.width());
        logo->setScaledContents(true);

        // this does nothing
        //        QPalette palette;
        //        palette.setColor(QPalette::Text, Qt::white);
        //        palette.setColor(QPalette::Link, "#a5cdff");
        //        palette.setColor(QPalette::LinkVisited, "#a5cdff");

        auto created = layout.emplace<QLabel>();
        created->setText(
            "Twitch Chat Client created by <a href=\"https://github.com/fourtf\">fourtf</a>");
        created->setTextFormat(Qt::RichText);
        created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                         Qt::LinksAccessibleByKeyboard |
                                         Qt::LinksAccessibleByKeyboard);
        created->setOpenExternalLinks(true);
        //        created->setPalette(palette);

        auto github = layout.emplace<QLabel>();
        github->setText(
            "<a href=\"https://github.com/fourtf/chatterino2\">Chatterino on Github</a>");
        github->setTextFormat(Qt::RichText);
        github->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByKeyboard |
                                        Qt::LinksAccessibleByKeyboard);
        github->setOpenExternalLinks(true);
        //        github->setPalette(palette);
    }
    layout->addStretch(1);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
