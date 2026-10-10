// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/StreamLinkErrorPopup.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Window.hpp"

namespace {

class ErrorTextView : public QTextEdit
{
public:
    ErrorTextView(const QString &errorMessage, QWidget *parent)
        : QTextEdit(parent)
    {
        this->setPlainText(errorMessage);
        this->setReadOnly(true);
        this->setLineWrapMode(QTextEdit::NoWrap);
    }
};

}  // namespace

namespace chatterino {

StreamLinkErrorPopup::StreamLinkErrorPopup(const QString &url,
                                           const QString &standardOutput,
                                           const QString &standardError)
    : BasePopup(
          {
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
          },
          static_cast<QWidget *>(&getApp()->getWindows()->getMainWindow()))
{
    this->ui_.vbox = new QVBoxLayout(this);
    this->ui_.infoLabel = new QLabel(
        QString("<b>Streamlink encountered an error while opening %1</b>")
            .arg(url),
        this);
    this->ui_.stdoutLabel = new QLabel("Standard Output:", this);
    this->ui_.stdoutTextEdit = new ErrorTextView(standardOutput, this);
    this->ui_.stderrLabel = new QLabel("Standard Error:", this);
    this->ui_.stderrTextEdit = new ErrorTextView(standardError, this);
    this->ui_.buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);

    QObject::connect(this->ui_.buttonBox, &QDialogButtonBox::accepted, this,
                     &StreamLinkErrorPopup::okButtonClicked);

    this->ui_.vbox->addWidget(this->ui_.infoLabel);
    this->ui_.vbox->addWidget(this->ui_.stdoutLabel);
    this->ui_.vbox->addWidget(this->ui_.stdoutTextEdit);
    this->ui_.vbox->addWidget(this->ui_.stderrLabel);
    this->ui_.vbox->addWidget(this->ui_.stderrTextEdit);
    this->ui_.vbox->addWidget(this->ui_.buttonBox);

    this->setLayout(this->ui_.vbox);
}

void StreamLinkErrorPopup::showError(const QString &url,
                                     const QString &standardOutput,
                                     const QString &standardError)
{
    auto *instance =
        new StreamLinkErrorPopup(url, standardOutput, standardError);

    instance->window()->setWindowTitle("Chatterino - streamlink error");
    instance->setAttribute(Qt::WA_DeleteOnClose, true);

    instance->show();
    instance->activateWindow();
    instance->raise();
}

void StreamLinkErrorPopup::keyPressEvent(QKeyEvent *e)
{
    if (this->handleEscape(e, this->ui_.buttonBox))
    {
        return;
    }
    if (this->handleEnter(e, this->ui_.buttonBox))
    {
        return;
    }

    BasePopup::keyPressEvent(e);
}

void StreamLinkErrorPopup::okButtonClicked()
{
    this->close();
}

}  // namespace chatterino
