#include "WelcomeDialog.hpp"

namespace chatterino {

WelcomeDialog::WelcomeDialog()
    : BaseWindow(BaseWindow::EnableCustomFrame)
{
    this->setWindowTitle("Chatterino quick setup");
}

}  // namespace chatterino
