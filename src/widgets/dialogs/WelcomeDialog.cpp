#include "widgets/dialogs/WelcomeDialog.hpp"

namespace chatterino {

WelcomeDialog::WelcomeDialog()
    : BaseWindow({BaseWindow::EnableCustomFrame, BaseWindow::DisableLayoutSave})
{
    this->setWindowTitle("Chatterino quick setup");
}

}  // namespace chatterino
