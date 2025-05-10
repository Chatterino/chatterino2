#include "EditUserNotesDialog.hpp"

#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"

#include <QDialogButtonBox>
#include <QTextEdit>

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
    this->setScaleIndependentSize(500, 350);

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

    this->themeChangedEvent();
}

void EditUserNotesDialog::setNotes(const QString &notes)
{
    this->textEdit_->setPlainText(notes);
}

void EditUserNotesDialog::updateWindowTitle(const QString &displayUsername)
{
    this->setWindowTitle("Editing notes for " + displayUsername);
}

void EditUserNotesDialog::showEvent(QShowEvent *event)
{
    this->textEdit_->setFocus(Qt::FocusReason::ActiveWindowFocusReason);

    BasePopup::showEvent(event);
}

void EditUserNotesDialog::themeChangedEvent()
{
    if (!this->theme)
    {
        return;
    }

    auto palette = this->palette();

    palette.setColor(QPalette::Window,
                     this->theme->tabs.selected.backgrounds.regular);
    palette.setColor(QPalette::Base, getTheme()->splits.background);
    palette.setColor(QPalette::Text, getTheme()->window.text);

    this->setPalette(palette);

    if (this->textEdit_)
    {
        this->textEdit_->setPalette(palette);
    }
}

}  // namespace chatterino
