#include "AboutPage.hpp"

#include "common/Modes.hpp"
#include "common/QLogging.hpp"
#include "common/Version.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QTextEdit>
#include <QTextStream>
#include <QVBoxLayout>

#define PIXMAP_WIDTH 500

#define LINK_CHATTERINO_WIKI "https://wiki.chatterino.com"
#define LINK_DONATE "https://streamelements.com/fourtf/tip"
#define LINK_CHATTERINO_FEATURES "https://chatterino.com/#features"
#define LINK_CHATTERINO_DISCORD "https://discord.gg/7Y5AYhAK4z"

namespace chatterino {

AboutPage::AboutPage()
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
        if (pixmap.width() != 0)
        {
            logo->setFixedSize(PIXMAP_WIDTH,
                               PIXMAP_WIDTH * pixmap.height() / pixmap.width());
        }
        logo->setScaledContents(true);

        // Version
        auto versionInfo = layout.emplace<QGroupBox>("Version");
        {
            auto vbox = versionInfo.emplace<QVBoxLayout>();
            auto version = Version::instance();

            auto label = vbox.emplace<QLabel>(version.buildString() + "<br>" +
                                              version.runningString());
            label->setOpenExternalLinks(true);
            label->setTextInteractionFlags(Qt::TextBrowserInteraction);

            auto disclaimer = vbox.emplace<QLabel>(
                "Send bugs to the <a "
                "href=https://github.com/mm2pl/dankerino>Dankerino repo</a> "
                "not upstream Chatterino.");
            disclaimer->setStyleSheet("color: red");
            disclaimer->setOpenExternalLinks(true);
        }

        // About Chatterino
        auto aboutChatterino = layout.emplace<QGroupBox>("About Chatterino...");
        {
            auto l = aboutChatterino.emplace<QVBoxLayout>();

            // clang-format off
            l.emplace<QLabel>("Chatterino Wiki can be found <a href=\"" LINK_CHATTERINO_WIKI "\">here</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("All about Chatterino's <a href=\"" LINK_CHATTERINO_FEATURES "\">features</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Join the official Chatterino <a href=\"" LINK_CHATTERINO_DISCORD "\">Discord</a>")->setOpenExternalLinks(true);
            // clang-format on
        }

        // Licenses
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
            addLicense(form.getElement(), "RapidJson", "https://rapidjson.org/",
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
#ifndef NO_QTKEYCHAIN
            addLicense(form.getElement(), "QtKeychain",
                       "https://github.com/frankosterfeld/qtkeychain",
                       ":/licenses/qtkeychain.txt");
#endif
            addLicense(form.getElement(), "lrucache",
                       "https://github.com/lamerman/cpp-lru-cache",
                       ":/licenses/lrucache.txt");
            addLicense(form.getElement(), "magic_enum",
                       "https://github.com/Neargye/magic_enum",
                       ":/licenses/magic_enum.txt");
            addLicense(form.getElement(), "semver",
                       "https://github.com/Neargye/semver",
                       ":/licenses/semver.txt");
            addLicense(form.getElement(), "miniaudio",
                       "https://github.com/mackron/miniaudio",
                       ":/licenses/miniaudio.txt");
#ifdef CHATTERINO_HAVE_PLUGINS
            addLicense(form.getElement(), "lua", "https://lua.org",
                       ":/licenses/lua.txt");
            addLicense(form.getElement(), "Fluent icons",
                       "https://github.com/microsoft/fluentui-system-icons",
                       ":/licenses/fluenticons.txt");
#endif
#ifdef CHATTERINO_WITH_CRASHPAD
            addLicense(form.getElement(), "sentry-crashpad",
                       "https://github.com/getsentry/crashpad",
                       ":/licenses/crashpad.txt");
#endif
        }

        // Attributions
        auto attributions = layout.emplace<QGroupBox>("Attributions...");
        {
            auto l = attributions.emplace<QVBoxLayout>();

            // clang-format off
            l.emplace<QLabel>("Twemoji emojis provided by <a href=\"https://github.com/twitter/twemoji\">Twitter's Twemoji</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Facebook emojis provided by <a href=\"https://facebook.com\">Facebook</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Apple emojis provided by <a href=\"https://apple.com\">Apple</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Google emojis provided by <a href=\"https://google.com\">Google</a>")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Emoji datasource provided by <a href=\"https://www.iamcal.com/\">Cal Henderson</a>"
                              "(<a href=\"https://github.com/iamcal/emoji-data/blob/master/LICENSE\">show license</a>)")->setOpenExternalLinks(true);
            l.emplace<QLabel>("GraphQL Logo is licensed under <a href=\"https://github.com/graphql/graphql-spec/issues/398#issuecomment-426844088\">CC-BY-4.0</a>)")->setOpenExternalLinks(true);
            l.emplace<QLabel>("Twitch emote data provided by <a href=\"https://emotes.raccatta.cc/\">emotes.raccatta.cc</a>")->setOpenExternalLinks(true);
            // clang-format on
        }

        // Contributors
        auto contributors = layout.emplace<QGroupBox>("Contributors");
        {
            auto l = contributors.emplace<QVBoxLayout>();

            QFile contributorsFile(":/contributors.txt");
            contributorsFile.open(QFile::ReadOnly);

            QTextStream stream(&contributorsFile);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            // Default encoding of QTextStream is already UTF-8
#else
            stream.setCodec("UTF-8");
#endif

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
                    qCDebug(chatterinoWidget)
                        << "Missing parts in line" << line;
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
    auto advancedButton =
        layout.emplace<QPushButton>("Enable advanced Dankerino settings");
    QObject::connect(
        advancedButton.getElement(), &QPushButton::clicked,
        [bttn = advancedButton.getElement()] {
            getSettings()->dankerinoThreeLetterApiEasterEgg.setValue(true);
            bttn->setText("now restart your client");
            bttn->setEnabled(false);
        });

    layout->addStretch(1);
}

void AboutPage::addLicense(QFormLayout *form, const QString &name,
                           const QString &website, const QString &licenseLink)
{
    auto *a = new QLabel("<a href=\"" + website + "\">" + name + "</a>");
    a->setOpenExternalLinks(true);
    auto *b = new QLabel("<a href=\"" + licenseLink + "\">show license</a>");
    QObject::connect(
        b, &QLabel::linkActivated, [parent = this, name, licenseLink] {
            auto window = new BasePopup({BaseWindow::Flags::EnableCustomFrame,
                                         BaseWindow::DisableLayoutSave},
                                        parent);
            window->setWindowTitle("Chatterino - License for " + name);
            window->setAttribute(Qt::WA_DeleteOnClose);
            auto layout = new QVBoxLayout();
            auto *edit = new QTextEdit;

            QFile file(licenseLink);
            file.open(QIODevice::ReadOnly);
            edit->setText(file.readAll());
            edit->setReadOnly(true);

            layout->addWidget(edit);

            window->getLayoutContainer()->setLayout(layout);
            window->show();
        });

    form->addRow(a, b);
}

}  // namespace chatterino
