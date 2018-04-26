#include "moderationpage.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "singletons/pathmanager.hpp"
#include "util/layoutcreator.hpp"

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
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    singletons::PathManager &pathManager = singletons::PathManager::getInstance();
    util::LayoutCreator<ModerationPage> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();
    {
        // Logs (copied from LoggingMananger)
        auto logPath = pathManager.logsFolderPath;

        auto created = layout.emplace<QLabel>();
        created->setText("Logs are saved to " + CreateLink(logPath, true));
        created->setTextFormat(Qt::RichText);
        created->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                         Qt::LinksAccessibleByKeyboard |
                                         Qt::LinksAccessibleByKeyboard);
        created->setOpenExternalLinks(true);
        layout.append(this->createCheckBox("Enable logging", settings.enableLogging));

        layout->addStretch(1);
        // Logs end

        // clang-format off
        auto label = layout.emplace<QLabel>("Click the moderation mod button (<img width='18' height='18' src=':/images/moderatormode_disabled.png'>) in a channel that you moderate to enable moderator mode.<br>");
        label->setWordWrap(true);
        label->setStyleSheet("color: #bbb");
        // clang-format on

        auto form = layout.emplace<QFormLayout>();
        {
            form->addRow("Action on timed out messages (unimplemented):",
                         this->createComboBox({"Disable", "Hide"}, settings.timeoutAction));
        }

        auto modButtons =
            layout.emplace<QGroupBox>("Custom moderator buttons").setLayoutType<QVBoxLayout>();
        {
            auto label2 =
                modButtons.emplace<QLabel>("One action per line. {user} will be replaced with the "
                                           "username.<br>Example `/timeout {user} 120`<br>");
            label2->setWordWrap(true);

            auto text = modButtons.emplace<QTextEdit>().getElement();

            text->setPlainText(settings.moderationActions);

            QObject::connect(text, &QTextEdit::textChanged, this,
                             [this] { this->itemsChangedTimer.start(200); });

            QObject::connect(&this->itemsChangedTimer, &QTimer::timeout, this, [text, &settings]() {
                settings.moderationActions = text->toPlainText();
            });
        }
    }

    // ---- misc
    this->itemsChangedTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
