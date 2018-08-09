#include "NotificationPage.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "src/controllers/notifications/NotificationController.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListView>
#include <QTableView>
#include <QTimer>

namespace chatterino {

NotificationPage::NotificationPage()
    : SettingsPage("Notifications", "")
{
    LayoutCreator<NotificationPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        auto tabs = layout.emplace<QTabWidget>();
        {
            auto settings = tabs.appendTab(new QVBoxLayout, "Options");
            {
                settings.emplace<QLabel>(
                    "Enable for channel next to channel name");
                settings.append(this->createCheckBox(
                    "Flash taskbar",
                    getApp()->settings->notificationFlashTaskbar));
                settings.append(this->createCheckBox(
                    "Playsound", getApp()->settings->notificationPlaySound));

                settings->addStretch(1);
            }
            auto channels = tabs.appendTab(new QVBoxLayout, "Channels");
            {
                EditableModelView *view =
                    channels
                        .emplace<EditableModelView>(
                            getApp()->notifications->createModel(nullptr))
                        .getElement();
                view->setTitles({"Channels"});

                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                view->addButtonPressed.connect([] {
                    getApp()->notifications->addChannelNotification("channel");
                });
            }
        }
    }
}
}  // namespace chatterino
