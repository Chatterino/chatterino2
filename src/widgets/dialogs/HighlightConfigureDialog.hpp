#pragma once

#include "controllers/highlights/HighlightPhrase.hpp"

#include <QDialog>

namespace Ui {

class HighlightConfigureDialog;

}  // namespace Ui

namespace chatterino {

struct IrcServerData;

class HighlightConfigureDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HighlightConfigureDialog(HighlightPhrase phrase, QWidget *parent);

    HighlightConfigureDialog(const HighlightConfigureDialog &) = delete;
    HighlightConfigureDialog(HighlightConfigureDialog &&) = delete;
    HighlightConfigureDialog &operator=(const HighlightConfigureDialog &) =
        delete;
    HighlightConfigureDialog &operator=(HighlightConfigureDialog &&) = delete;
    ~HighlightConfigureDialog() override;

private:
    Ui::HighlightConfigureDialog *ui_;
};

}  // namespace chatterino
