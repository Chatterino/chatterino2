#pragma once

#include "controllers/hotkeys/Hotkey.hpp"

#include <QDialog>

#include <memory>

namespace Ui {
class EditHotkeyDialog;
}

namespace chatterino {

class EditHotkeyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditHotkeyDialog(const std::shared_ptr<Hotkey> data,
                              bool isAdd = false, QWidget *parent = nullptr);
    ~EditHotkeyDialog();

    std::shared_ptr<Hotkey> data();

protected slots:
    void afterEdit();
    void updatePossibleActions();
    void updateArgumentsInput();

private:
    void showEditError(QString errorText);

    Ui::EditHotkeyDialog *ui_;
    std::shared_ptr<Hotkey> data_;

    bool shownSingleKeyWarning = false;
};

}  // namespace chatterino
