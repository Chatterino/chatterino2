// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/StreamLinkErrorPopup.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Window.hpp"

namespace chatterino {

StreamLinkErrorPopup::StreamLinkErrorPopup(const QString &url,
                                           const QString &errorMessage)
    : BasePopup(
          {
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
          },
          static_cast<QWidget *>(&getApp()->getWindows()->getMainWindow()))
{
    this->ui_.vbox = new QVBoxLayout(this);
    this->ui_.label = new QLabel(
        QString("<b>Streamlink encountered an error while opening %1</b>")
            .arg(url),
        this);
    this->ui_.textEdit = new QTextEdit(this);
    this->ui_.textEdit->setPlainText(errorMessage);
    this->ui_.textEdit->setReadOnly(true);
    this->ui_.textEdit->setLineWrapMode(QTextEdit::NoWrap);
    this->ui_.buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);

    QObject::connect(this->ui_.buttonBox, &QDialogButtonBox::accepted, this,
                     &StreamLinkErrorPopup::okButtonClicked);

    this->ui_.vbox->addWidget(this->ui_.label);
    this->ui_.vbox->addWidget(this->ui_.textEdit);
    this->ui_.vbox->addWidget(this->ui_.buttonBox);

    this->setLayout(this->ui_.vbox);
}

void StreamLinkErrorPopup::showError(const QString &url,
                                     const QString &errorMessage)
{
    auto *instance = new StreamLinkErrorPopup(url, errorMessage);

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
