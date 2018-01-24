#include "moderationpage.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "util/layoutcreator.hpp"

namespace chatterino {
namespace widgets {
namespace settingspages {
ModerationPage::ModerationPage()
    : SettingsPage("Moderation", "")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<ModerationPage> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();
    {
        // clang-format off
        auto label = layout.emplace<QLabel>("Click the moderation mod button (<img width='18' height='18' src=':/images/moderatormode_disabled.png'>) in a channel that you moderate to enable moderator mode.<br>");
        label->setWordWrap(true);
        label->setStyleSheet("color: #bbb");
        // clang-format on

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
