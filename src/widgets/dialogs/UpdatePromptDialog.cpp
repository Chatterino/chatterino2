#include "UpdatePromptDialog.hpp"

#include "util/LayoutCreator.hpp"
#include "widgets/Label.hpp"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {

UpdatePromptDialog::UpdatePromptDialog()
    : BaseWindow(nullptr, BaseWindow::Flags(BaseWindow::Frameless | BaseWindow::TopMost |
                                            BaseWindow::EnableCustomFrame))
{
    auto layout = LayoutCreator<UpdatePromptDialog>(this).setLayoutType<QVBoxLayout>();

    layout.emplace<Label>("An update is available!");
    layout.emplace<Label>("Do you want to download and install it?");
    layout.emplace<Label>("This doesn't work yet!");

    auto buttons = layout.emplace<QDialogButtonBox>();
    auto install = buttons->addButton("Install", QDialogButtonBox::AcceptRole);
    auto dismiss = buttons->addButton("Dismiss", QDialogButtonBox::RejectRole);

    QObject::connect(install, &QPushButton::clicked, this, [this] { this->close(); });
    QObject::connect(dismiss, &QPushButton::clicked, this, [this] { this->close(); });
}

}  // namespace chatterino
