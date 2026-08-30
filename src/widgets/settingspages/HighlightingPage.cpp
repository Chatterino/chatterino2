// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/HighlightingPage.hpp"

#include "controllers/highlights/HighlightBlacklistModel.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightController.hpp"
#include "singletons/Settings.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/HighlightingWidget.hpp"

#include <QFileDialog>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QTabWidget>

namespace chatterino {

HighlightingPage::HighlightingPage()
{
    LayoutCreator<HighlightingPage> layoutCreator(this);

    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();
    {
        // TABS
        auto tabs = layout.emplace<QTabWidget>();
        {
            // HIGHLIGHTS
            tabs->addTab(new HighlightingWidget, "Highlights");

            auto disabledUsers =
                tabs.appendTab(new QVBoxLayout, "Blacklisted Users");
            {
                disabledUsers.emplace<QLabel>(
                    "Disable notification sounds and highlights from certain "
                    "users (e.g. bots).");
                EditableModelView *view =
                    disabledUsers
                        .emplace<EditableModelView>(
                            (new HighlightBlacklistModel(nullptr))
                                ->initialized(&getSettings()->blacklistedUsers))
                        .getElement();

                view->addRegexHelpLink();
                view->setTitles({"Username", "Enable\nregex"});
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    QHeaderView::Fixed);
                view->getTableView()->horizontalHeader()->setSectionResizeMode(
                    0, QHeaderView::Stretch);

                // fourtf: make class extrend BaseWidget and add this to
                // dpiChanged
                QTimer::singleShot(1, [view] {
                    view->getTableView()->resizeColumnsToContents();
                    view->getTableView()->setColumnWidth(0, 200);
                });

                // We can safely ignore this signal connection since we own the view
                std::ignore = view->addButtonPressed.connect([] {
                    getSettings()->blacklistedUsers.append(
                        HighlightBlacklistUser{"blacklisted user", false});
                });
            }
        }

        // MISC
        if (const auto missing =
                HighlightController::missingBillTinHighlights();
            !missing.empty())
        {
            auto missingHighlights =
                layout.emplace<QHBoxLayout>().withoutMargin();
            auto *lbl = missingHighlights
                            .emplace<QLabel>(
                                "You are missing some built-in highlights.")
                            .getElement();
            auto *btn =
                missingHighlights
                    .emplace<QPushButton>("Recreate built-in highlights")
                    .getElement();
            QObject::connect(btn, &QPushButton::clicked, [lbl, btn, missing] {
                HighlightController::recreateMissingBillTinHighlights(missing);
                lbl->hide();
                btn->hide();
            });
        }

        layout.append(createCheckBox(
            "Play highlight sound even when Chatterino is focused",
            getSettings()->highlightAlwaysPlaySound));
        layout.append(createCheckBox(
            "Flash taskbar only stops highlighting when Chatterino is focused",
            getSettings()->longAlerts));
    }
}

}  // namespace chatterino
