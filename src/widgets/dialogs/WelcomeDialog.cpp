#include "WelcomeDialog.hpp"

namespace chatterino {
namespace widgets {

WelcomeDialog::WelcomeDialog()
    : BaseWindow(nullptr, BaseWindow::EnableCustomFrame)
{
    this->setWindowTitle("Chatterino quick setup");
}

}  // namespace widgets
}  // namespace chatterino
