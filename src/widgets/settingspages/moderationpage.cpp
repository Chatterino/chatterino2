#include "moderationpage.hpp"

#include "application.hpp"
#include "singletons/loggingmanager.hpp"
#include "singletons/pathmanager.hpp"
#include "util/layoutcreator.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {
namespace settingspages {

inline QString CreateLink(const QString &url, bool file = false)
{
    if (file) {
        return QString("<a href=\"file:///" + url + "\"><span style=\"color: white;\">" + url +
                       "</span></a>");
    }

    return QString("<a href=\"" + url + "\"><span style=\"color: white;\">" + url + "</span></a>");
}

ModerationPage::ModerationPage()
    : SettingsPage("Moderation", "")
{
    auto app = getApp();
    util::LayoutCreator<ModerationPage> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();
    {
        // Logs (copied from LoggingMananger)

        auto created = layout.emplace<QLabel>();
        if (app->settings->logPath == "") {
            created->setText("Logs are saved to " +
                             CreateLink(QCoreApplication::applicationDirPath(), true));
        } else {
            created->setText("Logs are saved to " + CreateLink(app->settings->logPath, true));
        }
        created->setTextFormat(Qt::RichText);
        created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                         Qt::LinksAccessibleByKeyboard |
                                         Qt::LinksAccessibleByKeyboard);
        created->setOpenExternalLinks(true);
        layout.append(this->createCheckBox("Enable logging", app->settings->enableLogging));

        layout->addStretch(1);

        auto selectDir = layout.emplace<QPushButton>("Logs");

        QObject::connect(
            selectDir.getElement(), &QPushButton::clicked, this,
            [ this, created, app, dirMemory = QString{app->settings->logPath} ]() mutable {
                auto dirName = QFileDialog::getExistingDirectory();

                created->setText("Logs are saved to " + CreateLink(dirName, true));
                app->settings->customLogPath = true;
                if (dirName == "" && dirMemory == "") {
                    created->setText("Logs are saved to " +
                                     CreateLink(QCoreApplication::applicationDirPath(), true));
                    app->settings->customLogPath = false;
                } else if (dirName == "") {
                    dirName = dirMemory;
                    created->setText("Logs are saved to " + CreateLink(dirName, true));
                }
                app->settings->logPath = dirName;

                qDebug() << "dirMemory" << dirMemory;
                qDebug() << "dirName" << dirName;
                qDebug() << app->settings->logPath.getValue();
                qDebug() << app->settings->customLogPath.getValue();

                dirMemory = dirName;
                app->logging->refreshLoggingPath();

            });
        auto resetDir = layout.emplace<QPushButton>("Reset logpath");
        QObject::connect(
            resetDir.getElement(), &QPushButton::clicked, this, [this, created, app]() mutable {
                app->settings->logPath = "";
                created->setText("Logs are saved to " +
                                 CreateLink(QCoreApplication::applicationDirPath(), true));
                app->settings->customLogPath = false;
                app->logging->refreshLoggingPath();
            });

        // Logs end

        // clang-format off
        auto label = layout.emplace<QLabel>("Click the moderation mod button (<img width='18' height='18' src=':/images/moderatormode_disabled.png'>) in a channel that you moderate to enable moderator mode.<br>");
        label->setWordWrap(true);
        label->setStyleSheet("color: #bbb");
        // clang-format on

        auto form = layout.emplace<QFormLayout>();
        {
            form->addRow("Action on timed out messages (unimplemented):",
                         this->createComboBox({"Disable", "Hide"}, app->settings->timeoutAction));
        }

        auto modButtons =
            layout.emplace<QGroupBox>("Custom moderator buttons").setLayoutType<QVBoxLayout>();
        {
            auto label2 =
                modButtons.emplace<QLabel>("One action per line. {user} will be replaced with the "
                                           "username.<br>Example `/timeout {user} 120`<br>");
            label2->setWordWrap(true);

            auto text = modButtons.emplace<QTextEdit>().getElement();

            text->setPlainText(app->settings->moderationActions);

            QObject::connect(text, &QTextEdit::textChanged, this,
                             [this] { this->itemsChangedTimer.start(200); });

            QObject::connect(&this->itemsChangedTimer, &QTimer::timeout, this, [text, app]() {
                app->settings->moderationActions = text->toPlainText();
            });
        }
    }

    // ---- misc
    this->itemsChangedTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
