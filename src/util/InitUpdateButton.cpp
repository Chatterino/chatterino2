#include "util/InitUpdateButton.hpp"

#include "Application.hpp"
#include "widgets/dialogs/UpdateDialog.hpp"
#include "widgets/helper/Button.hpp"

namespace chatterino {

void initUpdateButton(Button &button,
                      pajlada::Signals::SignalHolder &signalHolder)
{
    button.hide();

    // show update prompt when clicking the button
    QObject::connect(&button, &Button::leftClicked, [&button] {
        auto *dialog = new UpdateDialog();
        dialog->setActionOnFocusLoss(BaseWindow::Delete);

        auto globalPoint = button.mapToGlobal(
            QPoint(int(-100 * button.scale()), button.height()));

        // Make sure that update dialog will not go off left edge of screen
        if (globalPoint.x() < 0)
        {
            globalPoint.setX(0);
        }

        dialog->moveTo(globalPoint, widgets::BoundsChecking::DesiredPosition);
        dialog->show();
        dialog->raise();

        // We can safely ignore the signal connection because the dialog will always
        // be destroyed before the button is destroyed, since it is destroyed on focus loss
        //
        // The button is either attached to a Notebook, or a Window frame
        std::ignore = dialog->buttonClicked.connect([&button](auto buttonType) {
            switch (buttonType)
            {
                case UpdateDialog::Dismiss: {
                    button.hide();
                }
                break;
                case UpdateDialog::Install: {
                    getApp()->getUpdates().installUpdates();
                }
                break;
            }
        });

        //        handle.reset(dialog);
        //        dialog->closing.connect([&handle] { handle.release(); });
    });

    // update image when state changes
    auto updateChange = [&button](auto) {
        button.setVisible(getApp()->getUpdates().shouldShowUpdateButton());

        const auto *imageUrl = getApp()->getUpdates().isError()
                                   ? ":/buttons/updateError.png"
                                   : ":/buttons/update.png";
        button.setPixmap(QPixmap(imageUrl));
    };

    updateChange(getApp()->getUpdates().getStatus());

    signalHolder.managedConnect(getApp()->getUpdates().statusUpdated,
                                [updateChange](auto status) {
                                    updateChange(status);
                                });
}

}  // namespace chatterino
