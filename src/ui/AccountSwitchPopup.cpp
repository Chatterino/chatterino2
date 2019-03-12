#include "ui/AccountSwitchPopup.hpp"

#include "ab/Column.hpp"
#include "ab/Row.hpp"
#include "ab/Space.hpp"
#include "ab/util/MakeWidget.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"

#include <QPushButton>

namespace chatterino::ui
{
    AccountSwitchPopup::AccountSwitchPopup()
    {
        this->setCenterLayout(ab::makeLayout<ab::Column>({
            new AccountSwitchWidget(this),
            ab::space(),
            ab::makeLayout<ab::Row>({
                ab::stretch(),
                ab::makeWidget<QPushButton>([&](auto w) {
                    w->setText("Manage Accounts");

                    QObject::connect(w, &QPushButton::clicked, this, []() {
                        SettingsDialog::showDialog(
                            SettingsDialogPreference::Accounts);
                    });
                }),
            }),
        }));
    }
}  // namespace chatterino::ui
