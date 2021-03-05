#pragma once
#include "controllers/hotkeys/Hotkey.hpp"

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
    //std::shared_ptr<Hotkey> afterEdit();

protected slots:
    void afterEdit();

private:
    void showEditError(QString errorText);

    Ui::EditHotkeyDialog *ui_;
    std::shared_ptr<Hotkey> data_;
};

}  // namespace chatterino
