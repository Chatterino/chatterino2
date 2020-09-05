#include "ChannelFilterEditorDialog.hpp"

#include "controllers/filters/parser/FilterParser.hpp"

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
    auto vbox = new QVBoxLayout(this);
    auto filterVbox = new QVBoxLayout;
    auto buttonBox = new QHBoxLayout;
    auto okButton = new QPushButton("OK");
    auto cancelButton = new QPushButton("Cancel");

    okButton->setDefault(true);
    cancelButton->setDefault(false);

    auto helpLabel =
        new QLabel(QString("<a href='%1'><span "
                           "style='color:#99f'>variable help</span></a>")
                       .arg("https://github.com/Chatterino/chatterino2/blob/"
                            "master/docs/Filters.md#variables"));
    helpLabel->setOpenExternalLinks(true);

    buttonBox->addWidget(helpLabel);
    buttonBox->addStretch(1);
    buttonBox->addWidget(cancelButton);
    buttonBox->addWidget(okButton);

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

    auto titleInput = new QLineEdit;
    titleInput->setPlaceholderText("Filter name");
    titleInput->setText("My filter");

    this->titleInput_ = titleInput;
    filterVbox->addWidget(titleInput);

    auto left = new ChannelFilterEditorDialog::ValueSpecifier;
    auto right = new ChannelFilterEditorDialog::ValueSpecifier;
    auto exp =
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

    this->typeCombo_->insertItems(0, {"Text", "Number", "Variable"});
    this->varCombo_->insertItems(0, filterparser::friendlyValidIdentifiers);

    this->layout_->addWidget(this->typeCombo_);
    this->layout_->addWidget(this->varCombo_, 1);
    this->layout_->addWidget(this->valueInput_, 1);
    this->layout_->setContentsMargins(5, 5, 5, 5);

    QObject::connect(  //
        this->typeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int index) {
            if (index == 2)
            {
                this->valueInput_->hide();
                this->varCombo_->show();
            }
            else
            {
                this->valueInput_->show();
                this->varCombo_->hide();
                if (index == 1)
                {
                    auto validator = new QIntValidator;
                    validator->setBottom(0);
                    this->valueInput_->setValidator(validator);
                }
                else
                {
                    this->valueInput_->setValidator(nullptr);
                }
            }
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
            filterparser::friendlyValidIdentifiers.at(
                filterparser::validIdentifiers.indexOf(value)));
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
            return filterparser::validIdentifiers.at(
                this->varCombo_->currentIndex());
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

    QObject::connect(  //
        this->opCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
        [this](int index) {
            // disable if set to "(nothing)"
            this->right_->setEnabled(realBinaryOps.at(index) != "");
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
    if (opText == "")
    {
        return this->left_->expressionText();
    }

    return QString("(%1) %2 (%3)")
        .arg(this->left_->expressionText())
        .arg(opText)
        .arg(this->right_->expressionText());
}

}  // namespace chatterino
