#include "AboutPage.hpp"

#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QVBoxLayout>

#define PIXMAP_WIDTH 500

namespace chatterino {
namespace widgets {
namespace settingspages {

AboutPage::AboutPage()
    : SettingsPage("About", ":/images/about.svg")
{
    util::LayoutCreator<AboutPage> layoutCreator(this);

    auto scroll = layoutCreator.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    util::removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();
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

        /*auto xd = layout.emplace<QGroupBox>("Created by...");
        {
            auto created = xd.emplace<QLabel>();
            {
                created->setText("Created by <a href=\"https://github.com/fourtf\">fourtf</a><br>"
                                 "with big help from pajlada.");
                created->setTextFormat(Qt::RichText);
                created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                                 Qt::LinksAccessibleByKeyboard |
                                                 Qt::LinksAccessibleByKeyboard);
                created->setOpenExternalLinks(true);
                //        created->setPalette(palette);
            }

            //            auto github = xd.emplace<QLabel>();
            //            {
            //                github->setText(
            //                    "<a href=\"https://github.com/fourtf/chatterino2\">Chatterino on
            //                    Github</a>");
            //                github->setTextFormat(Qt::RichText);
            //                github->setTextInteractionFlags(Qt::TextBrowserInteraction |
            //                                                Qt::LinksAccessibleByKeyboard |
            //                                                Qt::LinksAccessibleByKeyboard);
            //                github->setOpenExternalLinks(true);
            //                //        github->setPalette(palette);
            //            }
        }*/

        auto licenses = layout.emplace<QGroupBox>("Open source software used...");
        {
            auto form = licenses.emplace<QFormLayout>();

            addLicense(form.getElement(), "Qt Framework", "https://www.qt.io",
                       ":/licenses/qt_lgpl-3.0.txt");
            addLicense(form.getElement(), "Boost", "https://www.boost.org/",
                       ":/licenses/boost_boost.txt");
            addLicense(form.getElement(), "Fmt", "http://fmtlib.net/", ":/licenses/fmt_bsd2.txt");
            addLicense(form.getElement(), "LibCommuni", "https://github.com/communi/libcommuni",
                       ":/licenses/libcommuni_BSD3.txt");
            addLicense(form.getElement(), "OpenSSL", "https://www.openssl.org/",
                       ":/licenses/openssl.txt");
            addLicense(form.getElement(), "RapidJson", "http://rapidjson.org/",
                       ":/licenses/rapidjson.txt");
            addLicense(form.getElement(), "Pajlada/Settings", "https://github.com/pajlada/settings",
                       ":/licenses/pajlada_settings.txt");
            addLicense(form.getElement(), "Pajlada/Signals", "https://github.com/pajlada/signals",
                       ":/licenses/pajlada_signals.txt");
            addLicense(form.getElement(), "Websocketpp", "https://www.zaphoyd.com/websocketpp/",
                       ":/licenses/websocketpp.txt");
        }

        auto attributions = layout.emplace<QGroupBox>("Attributions...");
        {
            auto l = attributions.emplace<QVBoxLayout>();

            // clang-format off
            l.emplace<QLabel>("EmojiOne 2 and 3 emojis provided by <a href=\"https://www.emojione.com/\">EmojiOne</a>");
            l.emplace<QLabel>("Twemoji emojis provided by <a href=\"https://github.com/twitter/twemoji\">Twitter's Twemoji</a>");
            l.emplace<QLabel>("Facebook emojis provided by <a href=\"https://facebook.com\">Facebook</a>");
            l.emplace<QLabel>("Apple emojis provided by <a href=\"https://apple.com\">Apple</a>");
            l.emplace<QLabel>("Google emojis provided by <a href=\"https://google.com\">Google</a>");
            l.emplace<QLabel>("Messenger emojis provided by <a href=\"https://facebook.com\">Facebook</a>");
            l.emplace<QLabel>("Emoji datasource provided by <a href=\"https://www.iamcal.com/\">Cal Henderson</a>");
            // clang-format on
        }
    }

    layout->addStretch(1);
}

void AboutPage::addLicense(QFormLayout *form, const QString &name, const QString &website,
                           const QString &licenseLink)
{
    auto *a = new QLabel("<a href=\"" + website + "\">" + name + "</a>");
    a->setOpenExternalLinks(true);
    auto *b = new SignalLabel();
    b->setText("<a href=\"" + licenseLink + "\">show license</a>");
    b->setCursor(Qt::PointingHandCursor);
    QObject::connect(b, &SignalLabel::mouseUp, [licenseLink] {
        auto *edit = new QTextEdit;

        QFile file(licenseLink);
        file.open(QIODevice::ReadOnly);
        edit->setText(file.readAll());
        edit->setReadOnly(true);
        edit->show();
    });

    form->addRow(a, b);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
