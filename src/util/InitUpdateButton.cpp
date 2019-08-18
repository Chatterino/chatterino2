#include "InitUpdateButton.hpp"

#include "widgets/dialogs/UpdateDialog.hpp"
#include "widgets/helper/Button.hpp"

namespace chatterino {

void initUpdateButton(Button &button,
                      pajlada::Signals::SignalHolder &signalHolder)
{
    button.hide();

    // show update prompt when clicking the button
    QObject::connect(&button, &Button::leftClicked, [&button] {
        auto dialog = new UpdateDialog();
        dialog->setActionOnFocusLoss(BaseWindow::Delete);
        dialog->move(button.mapToGlobal(
            QPoint(int(-100 * button.scale()), button.height())));
        dialog->show();
        dialog->raise();

        dialog->buttonClicked.connect([&button](auto buttonType) {
            switch (buttonType)
            {
                case UpdateDialog::Dismiss:
                {
                    button.hide();
                }
                break;
                case UpdateDialog::Install:
                {
                    Updates::getInstance().installUpdates();
                }
                break;
            }
        });

        //        handle.reset(dialog);
        //        dialog->closing.connect([&handle] { handle.release(); });
    });

    // update image when state changes
    auto updateChange = [&button](auto) {
        button.setVisible(Updates::getInstance().shouldShowUpdateButton());

        auto imageUrl = Updates::getInstance().isError()
                            ? ":/buttons/updateError.png"
                            : ":/buttons/update.png";
        button.setPixmap(QPixmap(imageUrl));
    };

    updateChange(Updates::getInstance().getStatus());

    signalHolder.managedConnect(
        Updates::getInstance().statusUpdated,
        [updateChange](auto status) { updateChange(status); });
}

}  // namespace chatterino
