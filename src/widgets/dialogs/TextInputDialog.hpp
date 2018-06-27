#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace chatterino {

class TextInputDialog : public QDialog
{
    Q_OBJECT

public:
    TextInputDialog(QWidget *parent = nullptr);

    QString getText() const
    {
        return _lineEdit.text();
    }

    void setText(const QString &text)
    {
        _lineEdit.setText(text);
    }

    void highlightText();

private:
    QVBoxLayout _vbox;
    QLineEdit _lineEdit;
    QHBoxLayout _buttonBox;
    QPushButton _okButton;
    QPushButton _cancelButton;

private slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace chatterino
