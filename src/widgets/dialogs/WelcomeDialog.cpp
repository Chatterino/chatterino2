#include "WelcomeDialog.hpp"

namespace chatterino {

WelcomeDialog::WelcomeDialog()
    : BaseWindow(BaseWindow::DisableLayoutSave)
{
    this->setWindowTitle("Chatterino quick setup");
}

}  // namespace chatterino
