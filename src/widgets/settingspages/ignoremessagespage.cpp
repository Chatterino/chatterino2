#include "ignoremessagespage.hpp"

#include "util/layoutcreator.hpp"

#include <QLabel>
#include <QTextEdit>

namespace chatterino {
namespace widgets {
namespace settingspages {
IgnoreMessagesPage::IgnoreMessagesPage()
    : SettingsPage("Ignore Messages", ":/images/theme.svg")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<IgnoreMessagesPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>("Ignored keywords:");
    QTextEdit *textEdit = layout.emplace<QTextEdit>().getElement();

    textEdit->setPlainText(settings.ignoredKeywords);

    QObject::connect(textEdit, &QTextEdit::textChanged,
                     [this] { this->keywordsUpdated.start(200); });

    QObject::connect(&this->keywordsUpdated, &QTimer::timeout, [textEdit, &settings] {
        QString text = textEdit->toPlainText();

        settings.ignoredKeywords = text;
    });

    // ---- misc
    this->keywordsUpdated.setSingleShot(true);
}
}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
