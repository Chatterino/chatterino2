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

        auto globalPoint = button.mapToGlobal(
                    QPoint(int(-100 * button.scale()), button.height()));

        // Make sure that update dialog will not go off left edge of screen
        if (globalPoint.x() < 0) {
            globalPoint.setX(0);
        }

        dialog->move(globalPoint);
        dialog->show();
        dialog->raise();

        dialog->buttonClicked.connect([&button](auto buttonType) {
            switch (buttonType)
            {
                case UpdateDialog::Dismiss: {
                    button.hide();
                }
                break;
                case UpdateDialog::Install: {
                    Updates::instance().installUpdates();
                }
                break;
            }
        });

        //        handle.reset(dialog);
        //        dialog->closing.connect([&handle] { handle.release(); });
    });

    // update image when state changes
    auto updateChange = [&button](auto) {
        button.setVisible(Updates::instance().shouldShowUpdateButton());

        auto imageUrl = Updates::instance().isError()
                            ? ":/buttons/updateError.png"
                            : ":/buttons/update.png";
        button.setPixmap(QPixmap(imageUrl));
    };

    updateChange(Updates::instance().getStatus());

    signalHolder.managedConnect(
        Updates::instance().statusUpdated,
        [updateChange](auto status) { updateChange(status); });
}

}  // namespace chatterino
