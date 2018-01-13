#include "moderationpage.hpp"

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

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        // clang-format off
        auto label = layout.emplace<QLabel>("In channels that you moderate there is a button <insert image of button here> to enable moderation mode.");
        label->setWordWrap(true);
        // clang-format on

        auto text = layout.emplace<QTextEdit>().getElement();

        QObject::connect(text, &QTextEdit::textChanged, this,
                         [this] { this->itemsChangedTimer.start(200); });

        QObject::connect(&this->itemsChangedTimer, &QTimer::timeout, this,
                         [text, &settings]() { settings.moderationActions = text->toPlainText(); });

        settings.highlightUserBlacklist.connect([=](const QString &str, auto) {
            text->setPlainText(str);  //
        });
    }

    // ---- misc
    this->itemsChangedTimer.setSingleShot(true);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
