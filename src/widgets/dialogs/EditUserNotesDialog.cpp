#include "EditUserNotesDialog.hpp"

#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/buttons/SvgButton.hpp"
#include "widgets/MarkdownLabel.hpp"

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

    auto headerLayout = layout.emplace<QHBoxLayout>();

    auto previewCheckBox =
        headerLayout.emplace<QCheckBox>("Show Markdown Preview")
            .assign(&this->previewCheckBox_);

    auto *headingButton = headerLayout
                              .emplace<SvgButton>(SvgButton::Src{
                                  .dark = ":/buttons/heading-darkMode.svg",
                                  .light = ":/buttons/heading-lightMode.svg",
                              })
                              .getElement();
    QObject::connect(headingButton, &Button::leftClicked, [this] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto selectedText = cursor.selectedText();
            cursor.insertText("### " + selectedText);
        }
        else
        {
            cursor.insertText("### ");
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    headingButton->setToolTip("Insert a heading");

    auto *boldButton = headerLayout
                           .emplace<SvgButton>(SvgButton::Src{
                               .dark = ":/buttons/bold-darkMode.svg",
                               .light = ":/buttons/bold-lightMode.svg",
                           })
                           .getElement();
    QObject::connect(boldButton, &Button::leftClicked, [this] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto selectedText = cursor.selectedText();

            // QTextEdit uses this sequence for newlines
            if (selectedText.back() == "\u2029")
            {
                selectedText.chop(1);
            }
            cursor.insertText("**" + selectedText + "**" + "\u2029");

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 3);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                selectedText.length());
        }
        else
        {
            cursor.insertText("****");
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    boldButton->setToolTip("Make selected text bold");

    auto *italicButton = headerLayout
                             .emplace<SvgButton>(SvgButton::Src{
                                 .dark = ":/buttons/italic-darkMode.svg",
                                 .light = ":/buttons/italic-lightMode.svg",
                             })
                             .getElement();
    QObject::connect(italicButton, &Button::leftClicked, [this] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto selectedText = cursor.selectedText();

            // QTextEdit uses this for newlines
            if (selectedText.back() == "\u2029")
            {
                selectedText.chop(1);
            }
            cursor.insertText("*" + selectedText + "*" + "\u2029");

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                selectedText.length());
        }
        else
        {
            cursor.insertText("**");
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    italicButton->setToolTip("Make selected text italic");

    auto *quoteButton = headerLayout
                            .emplace<SvgButton>(SvgButton::Src{
                                .dark = ":/buttons/quote-darkMode.svg",
                                .light = ":/buttons/quote-lightMode.svg",
                            })
                            .getElement();
    QObject::connect(quoteButton, &Button::leftClicked, [this] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto selectedText = cursor.selectedText();
            cursor.insertText("> " + selectedText);
        }
        else
        {
            cursor.insertText("> ");
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    quoteButton->setToolTip("Insert a blockquote");

    auto *linkButton = headerLayout
                           .emplace<SvgButton>(SvgButton::Src{
                               .dark = ":/buttons/link-darkMode.svg",
                               .light = ":/buttons/link-lightMode.svg",
                           })
                           .getElement();
    QObject::connect(linkButton, &Button::leftClicked, [this] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto selectedText = cursor.selectedText();

            // QTextEdit uses this for newlines
            if (selectedText.back() == "\u2029")
            {
                selectedText.chop(1);
            }
            cursor.insertText("[" + selectedText + "](url)" + "\u2029");

            // select "url" for easy replacement
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 2);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 3);
        }
        else
        {
            cursor.insertText("[](url)");
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 3);
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    linkButton->setToolTip("Insert a hyperlink");

    auto *listButton = headerLayout
                           .emplace<SvgButton>(SvgButton::Src{
                               .dark = ":/buttons/bullet-list-darkMode.svg",
                               .light = ":/buttons/bullet-list-lightMode.svg",
                           })
                           .getElement();
    QObject::connect(listButton, &Button::leftClicked, [this] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto selectedText = cursor.selectedText();
            cursor.insertText("- " + selectedText);
        }
        else
        {
            cursor.insertText("- ");
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    listButton->setToolTip("Insert a bullet list item");

    auto splitter =
        layout.emplace<QSplitter>(Qt::Horizontal).assign(&this->splitter_);

    auto edit = splitter.emplace<QTextEdit>().assign(&this->textEdit_);

    auto preview =
        splitter.emplace<MarkdownLabel>().assign(&this->previewLabel_);
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
                             this->textEdit_->setFocus();
                         }
                         else
                         {
                             this->splitter_->setSizes({700, 0});
                             this->textEdit_->setFocus();
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
