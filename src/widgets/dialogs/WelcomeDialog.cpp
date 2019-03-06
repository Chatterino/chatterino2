#include "WelcomeDialog.hpp"

namespace chatterino
{
    WelcomeDialog::WelcomeDialog()
        : ab::BaseWindow(BaseWindow::EnableCustomFrame)
    {
        this->setWindowTitle("Chatterino quick setup");
    }

}  // namespace chatterino
