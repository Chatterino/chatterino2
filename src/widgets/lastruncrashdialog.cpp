#include "lastruncrashdialog.hpp"

#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

LastRunCrashDialog::LastRunCrashDialog()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(
        new QLabel("The application wasn't terminated properly last time it was executed."));

    this->setLayout(layout);
}

}  // namespace widgets
}  // namespace chatterino
