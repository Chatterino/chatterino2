#include "IrcConnectionEditor.hpp"
#include "ui_IrcConnectionEditor.h"

IrcConnectionEditor::IrcConnectionEditor(bool isAdd, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::IrcConnectionEditor)
{
    ui->setupUi(this);

    this->setWindowTitle(QString(isAdd ? "Add " : "Edit ") + "Irc Connection");
}

IrcConnectionEditor::~IrcConnectionEditor()
{
    delete ui;
}
