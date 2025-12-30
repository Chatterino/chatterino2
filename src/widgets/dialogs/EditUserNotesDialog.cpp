#include "EditUserNotesDialog.hpp"

#include "singletons/Theme.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/buttons/SvgButton.hpp"
#include "widgets/helper/CmdDeleteKeyFilter.hpp"
#include "widgets/MarkdownLabel.hpp"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QRegularExpression>
#include <QSplitter>
#include <QTextBlock>
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

    auto *headingButton = headerLayout
                              .emplace<SvgButton>(SvgButton::Src{
                                  .dark = ":/buttons/heading-darkMode.svg",
                                  .light = ":/buttons/heading-lightMode.svg",
                              })
                              .getElement();
    QObject::connect(headingButton, &Button::leftClicked, this, [&] {
        auto cursor = this->textEdit_->textCursor();
        const auto line = cursor.block().text();
        const auto pos = EditUserNotesDialog::currentWordPosition(cursor);
        const auto selectedText = cursor.selectedText();

        if (!selectedText.isEmpty() &&
            EditUserNotesDialog::isHeading(line, pos))
        {
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 4);
            cursor.removeSelectedText();

            // restore selection
            cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length()));
        }
        else if (cursor.hasSelection())
        {
            cursor.insertText("### " + selectedText);

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length()));
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
    QObject::connect(boldButton, &Button::leftClicked, this, [&] {
        auto cursor = this->textEdit_->textCursor();
        const auto line = cursor.block().text();
        const auto pos = cursor.columnNumber();
        auto selectedText = cursor.selectedText();

        if (!selectedText.isEmpty() && EditUserNotesDialog::isBold(line, pos))
        {
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                                1);  // un-select
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 2);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length() + 4));
            cursor.insertText(selectedText);

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length()));
        }
        else if (cursor.hasSelection())
        {
            auto appended = 0;
            QChar newLine(QChar::ParagraphSeparator);

            if (selectedText.back() == newLine)
            {
                selectedText.chop(1);
                cursor.insertText("**" + selectedText + "**" + newLine);
                appended = 3;
            }
            else
            {
                cursor.insertText("**" + selectedText + "**");
                appended = 2;
            }

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                                appended);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length()));
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
    QObject::connect(italicButton, &Button::leftClicked, this, [&] {
        auto cursor = this->textEdit_->textCursor();
        const auto line = cursor.block().text();
        const auto pos = cursor.columnNumber();
        auto selectedText = cursor.selectedText();

        if (!selectedText.isEmpty() && EditUserNotesDialog::isItalic(line, pos))
        {
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                                1);  // un-select
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, 1);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length() + 2));
            cursor.insertText(selectedText);

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length()));
        }
        else if (cursor.hasSelection())
        {
            auto appended = 0;
            QChar newLine(QChar::ParagraphSeparator);

            if (selectedText.back() == newLine)
            {
                selectedText.chop(1);
                cursor.insertText("*" + selectedText + "*" + newLine);
                appended = 2;
            }
            else
            {
                cursor.insertText("*" + selectedText + "*");
                appended = 1;
            }

            // restore selection
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                                appended);
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor,
                                static_cast<int>(selectedText.length()));
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
    QObject::connect(quoteButton, &Button::leftClicked, this, [&] {
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
    QObject::connect(linkButton, &Button::leftClicked, this, [&] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            auto appended = 0;
            auto selectedText = cursor.selectedText();
            QChar newLine(QChar::ParagraphSeparator);

            if (selectedText.back() == newLine)
            {
                selectedText.chop(1);
                cursor.insertText("[" + selectedText + "](url)" + newLine);
                appended = 2;
            }
            else
            {
                cursor.insertText("[" + selectedText + "](url)");
                appended = 1;
            }

            // select "url" for easy replacement
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor,
                                appended);
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
    QObject::connect(listButton, &Button::leftClicked, this, [&] {
        auto cursor = this->textEdit_->textCursor();
        if (cursor.hasSelection())
        {
            cursor.select(QTextCursor::LineUnderCursor);
            const auto selectedText = cursor.selectedText();
            cursor.insertText("- " + selectedText);
            cursor.movePosition(QTextCursor::StartOfLine,
                                QTextCursor::KeepAnchor, 0);
        }
        else
        {
            const auto pos = cursor.columnNumber();
            cursor.movePosition(QTextCursor::StartOfLine,
                                QTextCursor::MoveAnchor, 0);
            cursor.insertText("- ");
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
                                pos);
        }
        this->textEdit_->setTextCursor(cursor);
        this->textEdit_->setFocus();
    });
    listButton->setToolTip("Insert a bullet list item");

    headerLayout->addSpacing(175);  // group markdown toolbar buttons

    auto previewCheckBox =
        headerLayout.emplace<QCheckBox>("Show Markdown Preview")
            .assign(&this->previewCheckBox_);

    auto splitter =
        layout.emplace<QSplitter>(Qt::Horizontal).assign(&this->splitter_);

    auto edit = splitter.emplace<QTextEdit>().assign(&this->textEdit_);

    auto preview = splitter.emplace<MarkdownLabel>(this, QString())
                       .assign(&this->previewLabel_);
    preview->setWordWrap(true);
    preview->setPadding(QMargins(10, 10, 10, 10));

    this->splitter_->setSizes({350, 350});
    this->previewLabel_->setVisible(false);

    auto *shortcutFilter = new CmdDeleteKeyFilter(edit.getElement());
    edit->installEventFilter(shortcutFilter);

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

    assert(this->previewLabel_ != nullptr);

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

    this->previewLabel_->setPalette(palette);
}

void EditUserNotesDialog::updatePreview()
{
    assert(this->textEdit_ != nullptr);
    assert(this->previewLabel_ != nullptr);

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

int EditUserNotesDialog::currentWordPosition(const QTextCursor &cursor)
{
    QTextCursor temp = cursor;
    temp.movePosition(QTextCursor::StartOfWord, QTextCursor::MoveAnchor);

    return temp.columnNumber();
}

bool EditUserNotesDialog::isBold(const QString &line, const int pos)
{
    static QRegularExpression pattern(R"((\*{2,})(.*?)(\1))");

    QRegularExpressionMatchIterator iter = pattern.globalMatch(line);

    while (iter.hasNext())
    {
        QRegularExpressionMatch match = iter.next();
        const auto startIndex = match.capturedStart(2);
        const auto endIndex = match.capturedEnd(2);

        if (pos == startIndex || pos == endIndex)
        {
            return true;
        }
    }

    return false;
}

bool EditUserNotesDialog::isItalic(const QString &line, const int pos)
{
    static QRegularExpression pattern(R"(\*(.*?)\*)");

    QRegularExpressionMatchIterator iter = pattern.globalMatch(line);

    while (iter.hasNext())
    {
        QRegularExpressionMatch match = iter.next();
        const auto startIndex = match.capturedStart(1);
        const auto endIndex = match.capturedEnd(1);

        if (pos == startIndex || pos == endIndex)
        {
            return true;
        }
    }

    return false;
}

bool EditUserNotesDialog::isHeading(const QString &line, const int pos)
{
    static QRegularExpression pattern(R"(###\s)");

    QRegularExpressionMatch match = pattern.match(line, pos - 4);

    return match.hasMatch() && pos == match.capturedEnd();
}

}  // namespace chatterino
