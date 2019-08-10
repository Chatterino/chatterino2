//#include "LogsPage.hpp"

//#include "Application.hpp"
//#include "singletons/PathManager.hpp"

//#include <QFormLayout>
//#include <QVBoxLayout>

//#include "util/LayoutCreator.hpp"

// namespace chatterino {
// namespace widgets {
// namespace settingspages {

// inline QString CreateLink(const QString &url, bool file = false)
//{
//    if (file) {
//        return QString("<a href=\"file:///" + url + "\"><span style=\"color:
//        white;\">" + url +
//                       "</span></a>");
//    }

//    return QString("<a href=\"" + url + "\"><span style=\"color: white;\">" +
//    url +
//    "</span></a>");
//}

// LogsPage::LogsPage()
//    : SettingsPage("Logs", "")
//{
//    auto app = getApp();

//    LayoutCreator<LogsPage> layoutCreator(this);
//    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

//    auto logPath = getPaths()->logsFolderPath;

//    auto created = layout.emplace<QLabel>();
//    created->setText("Logs are saved to " + CreateLink(logPath, true));
//    created->setTextFormat(Qt::RichText);
//    created->setTextInteractionFlags(Qt::TextBrowserInteraction |
//    Qt::LinksAccessibleByKeyboard |
//                                     Qt::LinksAccessibleByKeyboard);
//    created->setOpenExternalLinks(true);
//    layout.append(this->createCheckBox("Enable logging",
//    getSettings()->enableLogging));

//    layout->addStretch(1);
//}

//}  // namespace settingspages
//}  // namespace widgets
//}  // namespace chatterino
