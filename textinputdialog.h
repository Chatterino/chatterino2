#ifndef TEXTINPUTDIALOG_H
#define TEXTINPUTDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

class TextInputDialog : public QDialog
{
    Q_OBJECT

public:
    TextInputDialog(QWidget *parent = NULL);

    QString
    text() const
    {
        return m_lineEdit.text();
    }

    void
    setText(const QString &text)
    {
        m_lineEdit.setText(text);
    }

private:
    QVBoxLayout m_vbox;
    QLineEdit m_lineEdit;
    QHBoxLayout m_buttonBox;
    QPushButton m_okButton;
    QPushButton m_cancelButton;

    void okButtonClicked();
    void cancelButtonClicked();
};

#endif  // TEXTINPUTDIALOG_H
