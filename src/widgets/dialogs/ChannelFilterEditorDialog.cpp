#include "widgets/dialogs/ChannelFilterEditorDialog.hpp"

#include "controllers/filters/lang/Tokenizer.hpp"

#include <QLabel>
#include <QPushButton>

namespace chatterino {

namespace {
    const QStringList friendlyBinaryOps = {
        "and", "or",       "+",           "-",         "*",        "/",
        "%",   "equals",   "not equals",  "<",         ">",        "<=",
        ">=",  "contains", "starts with", "ends with", "(nothing)"};
    const QStringList realBinaryOps = {
        "&&", "||",       "+",          "-",        "*", "/",
        "%",  "==",       "!=",         "<",        ">", "<=",
        ">=", "contains", "startswith", "endswith", ""};
}  // namespace

ChannelFilterEditorDialog::ChannelFilterEditorDialog(QWidget *parent)
    : QDialog(parent)
{
    auto *vbox = new QVBoxLayout(this);
    auto *filterVbox = new QVBoxLayout;
    auto *buttonBox = new QHBoxLayout;
    auto *okButton = new QPushButton("Ok");
    auto *cancelButton = new QPushButton("Cancel");

    okButton->setDefault(true);
    cancelButton->setDefault(false);

    auto *helpLabel =
        new QLabel(QString("<a href='%1'><span "
                           "style='color:#99f'>variable help</span></a>")
                       .arg("https://wiki.chatterino.com/Filters/#variables"));
    helpLabel->setOpenExternalLinks(true);

    buttonBox->addWidget(helpLabel);
    buttonBox->addStretch(1);
    buttonBox->addWidget(okButton);
    buttonBox->addWidget(cancelButton);

    QObject::connect(okButton, &QAbstractButton::clicked, [this] {
        this->accept();
        this->close();
    });
    QObject::connect(cancelButton, &QAbstractButton::clicked, [this] {
        this->reject();
        this->close();
    });

    this->setWindowFlags(
        (this->windowFlags() & ~(Qt::WindowContextHelpButtonHint)) |
        Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);
    this->setWindowTitle("Channel Filter Creator");

    auto *titleInput = new QLineEdit;
    titleInput->setPlaceholderText("Filter name");
    titleInput->setText("My filter");

    this->titleInput_ = titleInput;
    filterVbox->addWidget(titleInput);

    auto *left = new ChannelFilterEditorDialog::ValueSpecifier;
    auto *right = new ChannelFilterEditorDialog::ValueSpecifier;
    auto *exp =
        new ChannelFilterEditorDialog::BinaryOperationSpecifier(left, right);

    this->expressionSpecifier_ = exp;
    filterVbox->addLayout(exp->layout());
    vbox->addLayout(filterVbox);
    vbox->addLayout(buttonBox);

    // setup default values
    left->setType("Variable");
    left->setValue("message.content");
    exp->setOperation("contains");
    right->setType("Text");
    right->setValue("hello");
}

const QString ChannelFilterEditorDialog::getFilter() const
{
    return this->expressionSpecifier_->expressionText();
}

const QString ChannelFilterEditorDialog::getTitle() const
{
    return this->titleInput_->text();
}

ChannelFilterEditorDialog::ValueSpecifier::ValueSpecifier()
{
    this->typeCombo_ = new QComboBox;
    this->varCombo_ = new QComboBox;
    this->valueInput_ = new QLineEdit;
    this->layout_ = new QHBoxLayout;

    this->typeCombo_->insertItems(
        0, {"Constant Text", "Constant Number", "Variable"});

    this->varCombo_->insertItems(0, filters::VALID_IDENTIFIERS_MAP.values());

    this->layout_->addWidget(this->typeCombo_);
    this->layout_->addWidget(this->varCombo_, 1);
    this->layout_->addWidget(this->valueInput_, 1);
    this->layout_->setContentsMargins(5, 5, 5, 5);

    QObject::connect(
        this->typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int index) {
            const auto isNumber = (index == 1);
            const auto isVariable = (index == 2);

            this->valueInput_->setVisible(!isVariable);
            this->varCombo_->setVisible(isVariable);
            this->valueInput_->setValidator(
                isNumber ? (new QIntValidator(0, INT_MAX)) : nullptr);

            this->valueInput_->clear();
        });

    this->varCombo_->hide();
    this->typeCombo_->setCurrentIndex(0);
}

void ChannelFilterEditorDialog::ValueSpecifier::setEnabled(bool enabled)
{
    this->typeCombo_->setEnabled(enabled);
    this->varCombo_->setEnabled(enabled);
    this->valueInput_->setEnabled(enabled);
}

void ChannelFilterEditorDialog::ValueSpecifier::setType(const QString &type)
{
    this->typeCombo_->setCurrentText(type);
}

void ChannelFilterEditorDialog::ValueSpecifier::setValue(const QString &value)
{
    if (this->typeCombo_->currentIndex() == 2)
    {
        this->varCombo_->setCurrentText(
            filters::VALID_IDENTIFIERS_MAP.value(value));
    }
    else
    {
        this->valueInput_->setText(value);
    }
}

QLayout *ChannelFilterEditorDialog::ValueSpecifier::layout() const
{
    return this->layout_;
}

QString ChannelFilterEditorDialog::ValueSpecifier::expressionText()
{
    switch (this->typeCombo_->currentIndex())
    {
        case 0:  // text
            return QString("\"%1\"").arg(
                this->valueInput_->text().replace("\"", "\\\""));
        case 1:  // number
            return this->valueInput_->text();
        case 2:  // variable
            return filters::VALID_IDENTIFIERS_MAP.key(
                this->varCombo_->currentText());
        default:
            return "";
    }
}

ChannelFilterEditorDialog::BinaryOperationSpecifier::BinaryOperationSpecifier(
    ExpressionSpecifier *left, ExpressionSpecifier *right)
    : left_(left)
    , right_(right)
{
    this->opCombo_ = new QComboBox;
    this->layout_ = new QVBoxLayout;

    this->opCombo_->insertItems(0, friendlyBinaryOps);

    this->layout_->addLayout(this->left_->layout());
    this->layout_->addWidget(this->opCombo_);
    this->layout_->addLayout(this->right_->layout());
    this->layout_->setContentsMargins(5, 5, 5, 5);

    QObject::connect(
        this->opCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int index) {
            // disable if set to "(nothing)"
            this->right_->setEnabled(!realBinaryOps.at(index).isEmpty());
        });
}

void ChannelFilterEditorDialog::BinaryOperationSpecifier::setEnabled(
    bool enabled)
{
    this->opCombo_->setEnabled(enabled);
    this->left_->setEnabled(enabled);
    this->right_->setEnabled(enabled);
}

void ChannelFilterEditorDialog::BinaryOperationSpecifier::setOperation(
    const QString &op)
{
    this->opCombo_->setCurrentText(op);
}

QLayout *ChannelFilterEditorDialog::BinaryOperationSpecifier::layout() const
{
    return this->layout_;
}

QString ChannelFilterEditorDialog::BinaryOperationSpecifier::expressionText()
{
    QString opText = realBinaryOps.at(this->opCombo_->currentIndex());
    if (opText.isEmpty())
    {
        return this->left_->expressionText();
    }

    return QString("(%1 %2 %3)")
        .arg(this->left_->expressionText())
        .arg(opText)
        .arg(this->right_->expressionText());
}

}  // namespace chatterino
