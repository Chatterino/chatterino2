#pragma once

namespace chatterino {
class ChannelFilterEditorDialog : public QDialog
{
public:
    ChannelFilterEditorDialog(QWidget *parent = nullptr);

    const QString getFilter() const;
    const QString getTitle() const;

private:
    class ExpressionSpecifier
    {
    public:
        virtual QLayout *layout() const = 0;
        virtual QString expressionText() = 0;
        virtual void setEnabled(bool enabled) = 0;
    };

    class ValueSpecifier : public ExpressionSpecifier
    {
    public:
        ValueSpecifier();

        QLayout *layout() const override;
        QString expressionText() override;
        void setEnabled(bool enabled) override;

        void setType(const QString &type);
        void setValue(const QString &value);

    private:
        QComboBox *typeCombo_, *varCombo_;
        QHBoxLayout *layout_;
        QLineEdit *valueInput_;
    };

    class BinaryOperationSpecifier : public ExpressionSpecifier
    {
    public:
        BinaryOperationSpecifier(ExpressionSpecifier *left,
                                 ExpressionSpecifier *right);

        QLayout *layout() const override;
        QString expressionText() override;
        void setEnabled(bool enabled) override;

        void setOperation(const QString &op);

    private:
        QComboBox *opCombo_;
        QVBoxLayout *layout_;
        ExpressionSpecifier *left_, *right_;
    };

    QString startFilter_;
    ExpressionSpecifier *expressionSpecifier_;
    QLineEdit *titleInput_;
};
}  // namespace chatterino
