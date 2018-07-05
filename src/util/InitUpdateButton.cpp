#include "InitUpdateButton.hpp"

#include "widgets/dialogs/UpdatePromptDialog.hpp"
#include "widgets/helper/RippleEffectButton.hpp"

namespace chatterino {

void initUpdateButton(RippleEffectButton &button, std::unique_ptr<UpdatePromptDialog> &handle,
                      pajlada::Signals::SignalHolder &signalHolder)
{
    button.hide();

    // show update prompt when clicking the button
    QObject::connect(&button, &RippleEffectButton::clicked, [&button, &handle] {
        (void)(handle);

        auto dialog = new UpdatePromptDialog();
        dialog->setActionOnFocusLoss(BaseWindow::Delete);
        dialog->move(button.mapToGlobal(QPoint(int(-100 * button.getScale()), button.height())));
        dialog->show();
        dialog->raise();

        dialog->buttonClicked.connect([&button](auto buttonType) {
            switch (buttonType) {
                case UpdatePromptDialog::Dismiss: {
                    button.hide();
                } break;
            }
        });

#ifdef Q_OS_WIN
        handle.reset(dialog);
        dialog->closing.connect([&handle] { handle.release(); });
#endif
    });

    // update image when state changes
    auto updateChange = [&button](auto) {
        button.setVisible(Updates::getInstance().shouldShowUpdateButton());

        auto imageUrl = Updates::getInstance().isError() ? ":/images/download_update_error.png"
                                                         : ":/images/download_update.png";
        button.setPixmap(QPixmap(imageUrl));
    };

    updateChange(Updates::getInstance().getStatus());

    signalHolder.managedConnect(Updates::getInstance().statusUpdated,
                                [updateChange](auto status) { updateChange(status); });
}

}  // namespace chatterino
