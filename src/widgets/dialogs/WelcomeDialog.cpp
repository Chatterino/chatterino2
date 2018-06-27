#include "WelcomeDialog.hpp"

namespace chatterino {

WelcomeDialog::WelcomeDialog()
    : BaseWindow(nullptr, BaseWindow::EnableCustomFrame)
{
    this->setWindowTitle("Chatterino quick setup");
}

}  // namespace chatterino
