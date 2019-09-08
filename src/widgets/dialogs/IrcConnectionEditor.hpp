#pragma once

#include <QDialog>

namespace Ui {
class IrcConnectionEditor;
}

class IrcConnectionEditor : public QDialog
{
    Q_OBJECT

public:
    explicit IrcConnectionEditor(bool isAdd = false, QWidget *parent = nullptr);
    ~IrcConnectionEditor();

private:
    Ui::IrcConnectionEditor *ui;
};
