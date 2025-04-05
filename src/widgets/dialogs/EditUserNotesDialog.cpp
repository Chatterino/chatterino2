#include "EditUserNotesDialog.hpp"

#include "util/LayoutCreator.hpp"

namespace chatterino {

EditUserNotesDialog::EditUserNotesDialog(QWidget *parent)
    : BasePopup(
          {
              BaseWindow::EnableCustomFrame,
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
          },
          parent)
{
    this->setScaleIndependantSize(500, 350);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    auto edit = layout.emplace<QTextEdit>().assign(&this->textEdit_);

    layout
        .emplace<QDialogButtonBox>(QDialogButtonBox::Ok |
                                   QDialogButtonBox::Cancel)
        .connect(&QDialogButtonBox::accepted, this,
                 [this, edit = edit.getElement()] {
                     this->onOk.invoke(edit->toPlainText());
                     this->close();
                 })
        .connect(&QDialogButtonBox::rejected, this, [this] {
            this->close();
        });
}

void EditUserNotesDialog::setNotes(const QString &notes)
{
    this->textEdit_->setPlainText(notes);
}

void EditUserNotesDialog::updateWindowTitle(const QString &displayUsername)
{
    this->setWindowTitle("Editing notes for " + displayUsername);
}

}  // namespace chatterino
