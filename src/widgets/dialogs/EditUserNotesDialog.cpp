#include "EditUserNotesDialog.hpp"

#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Label.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QSplitter>
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
    this->setScaleIndependentSize(700, 450);

    auto layout = LayoutCreator<QWidget>(this->getLayoutContainer())
                      .setLayoutType<QVBoxLayout>();

    auto previewCheckBox = layout.emplace<QCheckBox>("Show Markdown Preview")
                               .assign(&this->previewCheckBox_);

    auto splitter =
        layout.emplace<QSplitter>(Qt::Horizontal).assign(&this->splitter_);

    auto edit = splitter.emplace<QTextEdit>().assign(&this->textEdit_);

    auto preview = splitter.emplace<Label>().assign(&this->previewLabel_);
    preview->setMarkdownEnabled(true);
    preview->setWordWrap(true);
    preview->setPadding(QMargins(10, 10, 10, 10));

    this->splitter_->setSizes({350, 350});
    this->previewLabel_->setVisible(false);

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

    // Connect preview toggle
    QObject::connect(this->previewCheckBox_, &QCheckBox::toggled, this,
                     [this](bool checked) {
                         this->previewLabel_->setVisible(checked);
                         if (checked)
                         {
                             this->updatePreview();
                             this->splitter_->setSizes({350, 350});
                         }
                         else
                         {
                             this->splitter_->setSizes({700, 0});
                         }
                     });

    // Connect text changes to preview update
    QObject::connect(this->textEdit_, &QTextEdit::textChanged, this, [this] {
        if (this->previewCheckBox_->isChecked())
        {
            this->updatePreview();
        }
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

    if (this->previewLabel_)
    {
        this->previewLabel_->setPalette(palette);
    }
}

void EditUserNotesDialog::updatePreview()
{
    if (this->previewLabel_ && this->textEdit_)
    {
        QString text = this->textEdit_->toPlainText();
        if (text.isEmpty())
        {
            this->previewLabel_->setText(
                "*Preview will appear here when you type markdown text...*");
        }
        else
        {
            this->previewLabel_->setText(text);
        }
    }
}

}  // namespace chatterino
