#include "AboutPage.hpp"

#include "common/Modes.hpp"
#include "common/Version.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#define PIXMAP_WIDTH 500

namespace chatterino {

AboutPage::AboutPage()
    : SettingsPage("About", ":/settings/about.svg")
{
    LayoutCreator<AboutPage> layoutCreator(this);

    auto scroll = layoutCreator.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();
    {
        QPixmap pixmap;
        pixmap.load(":/settings/aboutlogo.png");

        auto logo = layout.emplace<QLabel>().assign(&this->logo_);
        logo->setPixmap(pixmap);
        logo->setFixedSize(PIXMAP_WIDTH,
                           PIXMAP_WIDTH * pixmap.height() / pixmap.width());
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
                created->setText("Created by <a
        href=\"https://github.com/fourtf\">fourtf</a><br>" "with big help from
        pajlada."); created->setTextFormat(Qt::RichText);
                created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                                 Qt::LinksAccessibleByKeyboard |
                                                 Qt::LinksAccessibleByKeyboard);
                created->setOpenExternalLinks(true);
                //        created->setPalette(palette);
            }

            //            auto github = xd.emplace<QLabel>();
            //            {
            //                github->setText(
            //                    "<a
        href=\"https://github.com/fourtf/chatterino2\">Chatterino on
            //                    Github</a>");
            //                github->setTextFormat(Qt::RichText);
            // github->setTextInteractionFlags(Qt::TextBrowserInteraction |
            // Qt::LinksAccessibleByKeyboard |
            // Qt::LinksAccessibleByKeyboard);
            //                github->setOpenExternalLinks(true);
            //                //        github->setPalette(palette);
            //            }
        }*/

        auto versionInfo = layout.emplace<QGroupBox>("Version");
        {
            auto version = Version::instance();
            QString text = QString("%1 (commit %2%3)")
                               .arg(version.fullVersion())
                               .arg("<a "
                                    "href=\"https://github.com/Chatterino/"
                                    "chatterino2/commit/" +
                                    version.commitHash() + "\">" +
                                    version.commitHash() + "</a>")
                               .arg(Modes::instance().isNightly
                                        ? ", " + version.dateOfBuild()
                                        : "");

            auto versionLabel = versionInfo.emplace<QLabel>(text);
            versionLabel->setOpenExternalLinks(true);
            versionLabel->setTextInteractionFlags(Qt::TextSelectableByMouse |
                                                  Qt::LinksAccessibleByMouse);
        }

        auto licenses =
            layout.emplace<QGroupBox>("Open source software used...");
        {
            auto form = licenses.emplace<QFormLayout>();

            addLicense(form.getElement(), "Qt Framework", "https://www.qt.io",
                       ":/licenses/qt_lgpl-3.0.txt");
            addLicense(form.getElement(), "Boost", "https://www.boost.org/",
                       ":/licenses/boost_boost.txt");
            addLicense(form.getElement(), "LibCommuni",
                       "https://github.com/communi/libcommuni",
                       ":/licenses/libcommuni_BSD3.txt");
            addLicense(form.getElement(), "OpenSSL", "https://www.openssl.org/",
                       ":/licenses/openssl.txt");
            addLicense(form.getElement(), "RapidJson", "http://rapidjson.org/",
                       ":/licenses/rapidjson.txt");
            addLicense(form.getElement(), "Pajlada/Settings",
                       "https://github.com/pajlada/settings",
                       ":/licenses/pajlada_settings.txt");
            addLicense(form.getElement(), "Pajlada/Signals",
                       "https://github.com/pajlada/signals",
                       ":/licenses/pajlada_signals.txt");
            addLicense(form.getElement(), "Websocketpp",
                       "https://www.zaphoyd.com/websocketpp/",
                       ":/licenses/websocketpp.txt");
            addLicense(form.getElement(), "QtKeychain",
                       "https://github.com/frankosterfeld/qtkeychain",
                       ":/licenses/qtkeychain.txt");
        }

        auto attributions = layout.emplace<QGroupBox>("Attributions...");
        {
            auto l = attributions.emplace<QVBoxLayout>();

            // clang-format off
            l.emplace<QLabel>("EmojiOne 2 and 3 emojis provided by <a href=\"https://www.emojione.com/\">EmojiOne</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Twemoji emojis provided by <a href=\"https://github.com/twitter/twemoji\">Twitter's Twemoji</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Facebook emojis provided by <a href=\"https://facebook.com\">Facebook</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Apple emojis provided by <a href=\"https://apple.com\">Apple</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Google emojis provided by <a href=\"https://google.com\">Google</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Messenger emojis provided by <a href=\"https://facebook.com\">Facebook</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Emoji datasource provided by <a href=\"https://www.iamcal.com/\">Cal Henderson</a>"
                              "(<a href=\"https://github.com/iamcal/emoji-data/blob/master/LICENSE\">show license</a>)")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Twitch emote data provided by <a href=\"https://twitchemotes.com/\">twitchemotes.com</a> through the <a href=\"https://github.com/Chatterino/api\">Chatterino API</a>")->setOpenExternalLinks(true);
            // clang-format on
        }

        // Contributors
        auto contributors = layout.emplace<QGroupBox>("Contributors");
        {
            auto l = contributors.emplace<QVBoxLayout>();

            QFile contributorsFile(":/contributors.txt");
            contributorsFile.open(QFile::ReadOnly);

            QTextStream stream(&contributorsFile);
            stream.setCodec("UTF-8");

            QString line;

            while (stream.readLineInto(&line))
            {
                if (line.isEmpty() || line.startsWith('#'))
                {
                    continue;
                }

                QStringList contributorParts = line.split("|");

                if (contributorParts.size() != 4)
                {
                    qDebug() << "Missing parts in line" << line;
                    continue;
                }

                QString username = contributorParts[0].trimmed();
                QString url = contributorParts[1].trimmed();
                QString avatarUrl = contributorParts[2].trimmed();
                QString role = contributorParts[3].trimmed();

                auto *usernameLabel =
                    new QLabel("<a href=\"" + url + "\">" + username + "</a>");
                usernameLabel->setOpenExternalLinks(true);
                auto *roleLabel = new QLabel(role);

                auto contributorBox2 = l.emplace<QHBoxLayout>();

                const auto addAvatar = [&avatarUrl, &contributorBox2] {
                    if (!avatarUrl.isEmpty())
                    {
                        QPixmap avatarPixmap;
                        avatarPixmap.load(avatarUrl);

                        auto avatar = contributorBox2.emplace<QLabel>();
                        avatar->setPixmap(avatarPixmap);
                        avatar->setFixedSize(64, 64);
                        avatar->setScaledContents(true);
                    }
                };

                const auto addLabels = [&contributorBox2, &usernameLabel,
                                        &roleLabel] {
                    auto labelBox = new QVBoxLayout();
                    contributorBox2->addLayout(labelBox);

                    labelBox->addWidget(usernameLabel);
                    labelBox->addWidget(roleLabel);
                };

                addLabels();
                addAvatar();
            }
        }
    }

    auto buildInfo = QStringList();
    buildInfo += "Qt " QT_VERSION_STR;
#ifdef USEWINSDK
    buildInfo += "Windows SDK";
#endif
#ifdef _MSC_FULL_VER
    buildInfo += "MSVC " + QString::number(_MSC_FULL_VER, 10);
#endif

    auto buildText = QString("Built with " + buildInfo.join(", "));
    layout.emplace<QLabel>(buildText);

    layout->addStretch(1);
}

void AboutPage::addLicense(QFormLayout *form, const QString &name,
                           const QString &website, const QString &licenseLink)
{
    auto *a = new QLabel("<a href=\"" + website + "\">" + name + "</a>");
    a->setOpenExternalLinks(true);
    auto *b = new QLabel("<a href=\"" + licenseLink + "\">show license</a>");
    QObject::connect(b, &QLabel::linkActivated, [licenseLink] {
        auto *edit = new QTextEdit;

        QFile file(licenseLink);
        file.open(QIODevice::ReadOnly);
        edit->setText(file.readAll());
        edit->setReadOnly(true);
        edit->show();
    });

    form->addRow(a, b);
}

}  // namespace chatterino
