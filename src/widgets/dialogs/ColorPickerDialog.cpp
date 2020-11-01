#include "widgets/dialogs/ColorPickerDialog.hpp"

#include "providers/colors/ColorProvider.hpp"
#include "singletons/Theme.hpp"

namespace chatterino {

ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : BasePopup(BaseWindow::EnableCustomFrame, parent)
    , color_()
    , dialogConfirmed_(false)
{
    // This hosts the "business logic" and the dialog button box
    LayoutCreator<QWidget> layoutWidget(this->getLayoutContainer());
    auto layout = layoutWidget.setLayoutType<QVBoxLayout>().withoutMargin();

    // This hosts the business logic: color picker and predefined colors
    LayoutCreator<QWidget> contentCreator(new QWidget());
    auto contents = contentCreator.setLayoutType<QHBoxLayout>();

    // This hosts the predefined colors (and also the currently selected color)
    LayoutCreator<QWidget> predefCreator(new QWidget());
    auto predef = predefCreator.setLayoutType<QVBoxLayout>();

    // Recently used colors
    {
        LayoutCreator<QWidget> gridCreator(new QWidget());
        this->initRecentColors(gridCreator);

        predef.append(gridCreator.getElement());
    }

    // Default colors
    {
        LayoutCreator<QWidget> gridCreator(new QWidget());
        this->initDefaultColors(gridCreator);

        predef.append(gridCreator.getElement());
    }

    // Currently selected color
    {
        LayoutCreator<QWidget> curColorCreator(new QWidget());
        auto curColor = curColorCreator.setLayoutType<QHBoxLayout>();
        curColor.emplace<QLabel>("Selected:").assign(&this->ui_.selected.label);
        curColor.emplace<ColorButton>(initial).assign(
            &this->ui_.selected.color);

        predef.append(curColor.getElement());
    }

    contents.append(predef.getElement());

    // Color picker
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        // The actual color picker
        {
            LayoutCreator<QWidget> cpCreator(new QWidget());
            this->initColorPicker(cpCreator);

            vbox.append(cpCreator.getElement());
        }

        // Spin boxes
        {
            LayoutCreator<QWidget> sbCreator(new QWidget());
            this->initSpinBoxes(sbCreator);

            vbox.append(sbCreator.getElement());
        }

        // HTML color
        {
            LayoutCreator<QWidget> htmlCreator(new QWidget());
            this->initHtmlColor(htmlCreator);

            vbox.append(htmlCreator.getElement());
        }

        contents.append(obj.getElement());
    }

    layout.append(contents.getElement());

    // Dialog buttons
    auto buttons =
        layout.emplace<QHBoxLayout>().emplace<QDialogButtonBox>(this);
    {
        auto *button_ok = buttons->addButton(QDialogButtonBox::Ok);
        QObject::connect(button_ok, &QPushButton::clicked,
                         [=](bool) { this->ok(); });
        auto *button_cancel = buttons->addButton(QDialogButtonBox::Cancel);
        QObject::connect(button_cancel, &QAbstractButton::clicked,
                         [=](bool) { this->close(); });
    }

    this->themeChangedEvent();
    this->selectColor(initial, false);
}

ColorPickerDialog::~ColorPickerDialog()
{
    if (this->htmlColorValidator_)
    {
        this->htmlColorValidator_->deleteLater();
        this->htmlColorValidator_ = nullptr;
    }
}

QColor ColorPickerDialog::selectedColor() const
{
    if (!this->dialogConfirmed_)
    {
        // If the Cancel button was clicked, return the invalid color
        return QColor();
    }

    return this->color_;
}

void ColorPickerDialog::closeEvent(QCloseEvent *)
{
    this->closed.invoke(this->selectedColor());
}

void ColorPickerDialog::themeChangedEvent()
{
    BaseWindow::themeChangedEvent();

    QString textCol = this->theme->splits.input.text.name(QColor::HexRgb);
    QString bgCol = this->theme->splits.input.background.name(QColor::HexRgb);

    // Labels

    QString labelStyle = QString("color: %1;").arg(textCol);

    this->ui_.recent.label->setStyleSheet(labelStyle);
    this->ui_.def.label->setStyleSheet(labelStyle);
    this->ui_.selected.label->setStyleSheet(labelStyle);
    this->ui_.picker.htmlLabel->setStyleSheet(labelStyle);

    for (auto spinBoxLabel : this->ui_.picker.spinBoxLabels)
    {
        spinBoxLabel->setStyleSheet(labelStyle);
    }

    this->ui_.picker.htmlEdit->setStyleSheet(
        this->theme->splits.input.styleSheet);

    // Styling spin boxes is too much effort
}

void ColorPickerDialog::selectColor(const QColor &color, bool fromColorPicker)
{
    if (color == this->color_)
        return;

    this->color_ = color;

    // Update UI elements
    this->ui_.selected.color->setColor(this->color_);

    /*
     * Somewhat "ugly" hack to prevent feedback loop between widgets. Since
     * this method is private, I'm okay with this being ugly.
     */
    if (!fromColorPicker)
    {
        this->ui_.picker.colorPicker->setCol(this->color_.hslHue(),
                                             this->color_.hslSaturation());
        this->ui_.picker.luminancePicker->setCol(this->color_.hsvHue(),
                                                 this->color_.hsvSaturation(),
                                                 this->color_.value());
    }

    this->ui_.picker.spinBoxes[SpinBox::RED]->setValue(this->color_.red());
    this->ui_.picker.spinBoxes[SpinBox::GREEN]->setValue(this->color_.green());
    this->ui_.picker.spinBoxes[SpinBox::BLUE]->setValue(this->color_.blue());
    this->ui_.picker.spinBoxes[SpinBox::ALPHA]->setValue(this->color_.alpha());

    /*
     * Here, we are intentionally using HexRgb instead of HexArgb. Most online
     * sites (or other applications) will likely not include the alpha channel
     * in their output.
     */
    this->ui_.picker.htmlEdit->setText(this->color_.name(QColor::HexRgb));
}

void ColorPickerDialog::ok()
{
    this->dialogConfirmed_ = true;
    this->close();
}

void ColorPickerDialog::initRecentColors(LayoutCreator<QWidget> &creator)
{
    auto grid = creator.setLayoutType<QGridLayout>();

    auto label = this->ui_.recent.label = new QLabel("Recently used:");
    grid->addWidget(label, 0, 0, 1, -1);

    const auto recentColors = ColorProvider::instance().recentColors();
    auto it = recentColors.begin();
    size_t ind = 0;
    while (it != recentColors.end() && ind < MAX_RECENT_COLORS)
    {
        this->ui_.recent.colors.push_back(new ColorButton(*it, this));
        auto *button = this->ui_.recent.colors[ind];

        static_assert(RECENT_COLORS_PER_ROW != 0);
        const int rowInd = (ind / RECENT_COLORS_PER_ROW) + 1;
        const int columnInd = ind % RECENT_COLORS_PER_ROW;

        grid->addWidget(button, rowInd, columnInd);

        QObject::connect(button, &QPushButton::clicked,
                         [=] { this->selectColor(button->color(), false); });

        ++it;
        ++ind;
    }

    auto spacer =
        new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    grid->addItem(spacer, (ind / RECENT_COLORS_PER_ROW) + 2, 0, 1, 1,
                  Qt::AlignTop);
}

void ColorPickerDialog::initDefaultColors(LayoutCreator<QWidget> &creator)
{
    auto grid = creator.setLayoutType<QGridLayout>();

    auto label = this->ui_.def.label = new QLabel("Default colors:");
    grid->addWidget(label, 0, 0, 1, -1);

    const auto defaultColors = ColorProvider::instance().defaultColors();
    auto it = defaultColors.begin();
    size_t ind = 0;
    while (it != defaultColors.end())
    {
        this->ui_.def.colors.push_back(new ColorButton(*it, this));
        auto *button = this->ui_.def.colors[ind];

        const int rowInd = (ind / DEFAULT_COLORS_PER_ROW) + 1;
        const int columnInd = ind % DEFAULT_COLORS_PER_ROW;

        grid->addWidget(button, rowInd, columnInd);

        QObject::connect(button, &QPushButton::clicked,
                         [=] { this->selectColor(button->color(), false); });

        ++it;
        ++ind;
    }

    auto spacer =
        new QSpacerItem(40, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    grid->addItem(spacer, (ind / DEFAULT_COLORS_PER_ROW) + 2, 0, 1, 1,
                  Qt::AlignTop);
}

void ColorPickerDialog::initColorPicker(LayoutCreator<QWidget> &creator)
{
    this->setWindowTitle("Chatterino - color picker");
    auto cpPanel = creator.setLayoutType<QHBoxLayout>();

    /*
     * For some reason, LayoutCreator::emplace didn't work for these.
     * (Or maybe I was too dense to make it work.)
     * After trying to debug for 4 hours or so, I gave up and settled
     * for this solution.
     */
    auto *colorPicker = new QColorPicker(this);
    this->ui_.picker.colorPicker = colorPicker;

    auto *luminancePicker = new QColorLuminancePicker(this);
    this->ui_.picker.luminancePicker = luminancePicker;

    cpPanel.append(colorPicker);
    cpPanel.append(luminancePicker);

    QObject::connect(colorPicker, SIGNAL(newCol(int, int)), luminancePicker,
                     SLOT(setCol(int, int)));

    QObject::connect(
        luminancePicker, &QColorLuminancePicker::newHsv,
        [=](int h, int s, int v) {
            int alpha = this->ui_.picker.spinBoxes[SpinBox::ALPHA]->value();
            this->selectColor(QColor::fromHsv(h, s, v, alpha), true);
        });
}

void ColorPickerDialog::initSpinBoxes(LayoutCreator<QWidget> &creator)
{
    auto spinBoxes = creator.setLayoutType<QGridLayout>();

    auto *red = this->ui_.picker.spinBoxes[SpinBox::RED] =
        new QColSpinBox(this);
    auto *green = this->ui_.picker.spinBoxes[SpinBox::GREEN] =
        new QColSpinBox(this);
    auto *blue = this->ui_.picker.spinBoxes[SpinBox::BLUE] =
        new QColSpinBox(this);
    auto *alpha = this->ui_.picker.spinBoxes[SpinBox::ALPHA] =
        new QColSpinBox(this);

    // We need pointers to these for theme changes
    auto *redLbl = this->ui_.picker.spinBoxLabels[SpinBox::RED] =
        new QLabel("Red:");
    auto *greenLbl = this->ui_.picker.spinBoxLabels[SpinBox::GREEN] =
        new QLabel("Green:");
    auto *blueLbl = this->ui_.picker.spinBoxLabels[SpinBox::BLUE] =
        new QLabel("Blue:");
    auto *alphaLbl = this->ui_.picker.spinBoxLabels[SpinBox::ALPHA] =
        new QLabel("Alpha:");

    spinBoxes->addWidget(redLbl, 0, 0);
    spinBoxes->addWidget(red, 0, 1);

    spinBoxes->addWidget(greenLbl, 1, 0);
    spinBoxes->addWidget(green, 1, 1);

    spinBoxes->addWidget(blueLbl, 2, 0);
    spinBoxes->addWidget(blue, 2, 1);

    spinBoxes->addWidget(alphaLbl, 3, 0);
    spinBoxes->addWidget(alpha, 3, 1);

    for (size_t i = 0; i < SpinBox::END; ++i)
    {
        QObject::connect(
            this->ui_.picker.spinBoxes[i],
            QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
                this->selectColor(QColor(red->value(), green->value(),
                                         blue->value(), alpha->value()),
                                  false);
            });
    }
}

void ColorPickerDialog::initHtmlColor(LayoutCreator<QWidget> &creator)
{
    auto html = creator.setLayoutType<QGridLayout>();

    // Copied from Qt source for QColorShower
    static QRegularExpression regExp(
        QStringLiteral("#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})"));
    auto *validator = this->htmlColorValidator_ =
        new QRegularExpressionValidator(regExp, this);

    auto *htmlLabel = this->ui_.picker.htmlLabel = new QLabel("HTML:");
    auto *htmlEdit = this->ui_.picker.htmlEdit = new QLineEdit(this);

    htmlEdit->setValidator(validator);

    html->addWidget(htmlLabel, 0, 0);
    html->addWidget(htmlEdit, 0, 1);

    QObject::connect(htmlEdit, &QLineEdit::textEdited,
                     [=](const QString &text) {
                         QColor col(text);
                         if (col.isValid())
                             this->selectColor(col, false);
                     });
}

}  // namespace chatterino
