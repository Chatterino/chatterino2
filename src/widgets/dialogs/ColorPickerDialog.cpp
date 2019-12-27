#include "widgets/dialogs/ColorPickerDialog.hpp"

#include "providers/colors/ColorProvider.hpp"

#include <QColorDialog>

namespace chatterino {

ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
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
        auto grid = gridCreator.setLayoutType<QGridLayout>();

        grid->addWidget(new QLabel("Recently used:"), 0, 0, 1, -1);

        const auto recentColors = ColorProvider::instance().recentColors();
        auto it = recentColors.begin();
        size_t ind = 0;
        while (it != recentColors.end() && ind < MAX_RECENT_COLORS)
        {
            this->ui_.recentColors.push_back(new ColorButton(*it, this));
            auto *button = this->ui_.recentColors[ind];

            const int rowInd = (ind / RECENT_COLORS_PER_ROW) + 1;
            const int columnInd = ind % RECENT_COLORS_PER_ROW;

            grid->addWidget(button, rowInd, columnInd);

            QObject::connect(button, &QPushButton::clicked, [=] {
                this->selectColor(button->color(), false);
            });

            ++it;
            ++ind;
        }

        auto spacer = new QSpacerItem(40, 20, QSizePolicy::Minimum,
                                      QSizePolicy::Expanding);
        grid->addItem(spacer, (ind / RECENT_COLORS_PER_ROW) + 2, 0, 1, 1,
                      Qt::AlignTop);

        predef.append(gridCreator.getElement());
    }

    // Default colors
    {
        LayoutCreator<QWidget> gridCreator(new QWidget());
        auto grid = gridCreator.setLayoutType<QGridLayout>();

        grid->addWidget(new QLabel("Default colors:"), 0, 0, 1, -1);

        const auto defaultColors = ColorProvider::instance().defaultColors();
        auto it = defaultColors.begin();
        size_t ind = 0;
        while (it != defaultColors.end())
        {
            this->ui_.defaultColors.push_back(new ColorButton(*it, this));
            auto *button = this->ui_.defaultColors[ind];

            const int rowInd = (ind / DEFAULT_COLORS_PER_ROW) + 1;
            const int columnInd = ind % DEFAULT_COLORS_PER_ROW;

            grid->addWidget(button, rowInd, columnInd);

            QObject::connect(button, &QPushButton::clicked, [=] {
                this->selectColor(button->color(), false);
            });

            ++it;
            ++ind;
        }

        auto spacer = new QSpacerItem(40, 20, QSizePolicy::Minimum,
                                      QSizePolicy::Expanding);
        grid->addItem(spacer, (ind / DEFAULT_COLORS_PER_ROW) + 2, 0, 1, 1,
                      Qt::AlignTop);

        predef.append(gridCreator.getElement());
    }

    // Currently selected color
    {
        LayoutCreator<QWidget> curColorCreator(new QWidget());
        auto curColor = curColorCreator.setLayoutType<QHBoxLayout>();
        curColor.emplace<QLabel>("Selected:");
        curColor.emplace<ColorButton>(initial).assign(&this->ui_.selectedColor);

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
            auto cpPanel = cpCreator.setLayoutType<QHBoxLayout>();

            /*
             * For some reason, LayoutCreator::emplace didn't work for these.
             * (Or maybe I was too dense to make it work.)
             * After trying to debug for 4 hours or so, I gave up and settled
             * for this solution.
             */
            auto *colorPicker = new QColorPicker(this);
            this->ui_.colorPicker = colorPicker;

            auto *luminancePicker = new QColorLuminancePicker(this);
            this->ui_.luminancePicker = luminancePicker;

            cpPanel.append(colorPicker);
            cpPanel.append(luminancePicker);

            QObject::connect(colorPicker, SIGNAL(newCol(int, int)),
                             luminancePicker, SLOT(setCol(int, int)));

            QObject::connect(
                luminancePicker, &QColorLuminancePicker::newHsv,
                [=](int h, int s, int v) {
                    int alpha = this->ui_.spinBoxes[SpinBox::ALPHA]->value();
                    this->selectColor(QColor::fromHsv(h, s, v, alpha), true);
                });

            vbox.append(cpPanel.getElement());
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
            auto html = htmlCreator.setLayoutType<QGridLayout>();

            // Copied from Qt source for QColorShower
            QRegularExpression regExp(
                QStringLiteral("#([A-Fa-f0-9]{6}|[A-Fa-f0-9]{3})"));
            auto *validator = this->htmlColorValidator_ =
                new QRegularExpressionValidator(regExp, this);

            auto *htmlEdit = this->ui_.htmlEdit = new QLineEdit(this);

            htmlEdit->setValidator(validator);

            html->addWidget(new QLabel("HTML:"), 0, 0);
            html->addWidget(htmlEdit, 0, 1);

            QObject::connect(htmlEdit, &QLineEdit::textEdited,
                             [=](const QString &text) {
                                 QColor col(text);
                                 if (col.isValid())
                                     this->selectColor(col, false);
                             });

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
        // If the Cancel button was clicked, return the invalid color
        return QColor();

    return this->color_;
}

void ColorPickerDialog::closeEvent(QCloseEvent *)
{
    this->closed.invoke();
}

void ColorPickerDialog::selectColor(const QColor &color, bool fromColorPicker)
{
    if (color == this->color_)
        return;

    this->color_ = color;

    // Update UI elements
    this->ui_.selectedColor->setColor(this->color_);

    /*
     * Somewhat "ugly" hack to prevent feedback loop between widgets. Since
     * this method is private, I'm okay with this being ugly.
     */
    if (!fromColorPicker)
    {
        this->ui_.colorPicker->setCol(this->color_.hslHue(),
                                      this->color_.hslSaturation());
        this->ui_.luminancePicker->setCol(this->color_.hsvHue(),
                                          this->color_.hsvSaturation(),
                                          this->color_.value());
    }

    this->ui_.spinBoxes[SpinBox::RED]->setValue(this->color_.red());
    this->ui_.spinBoxes[SpinBox::GREEN]->setValue(this->color_.green());
    this->ui_.spinBoxes[SpinBox::BLUE]->setValue(this->color_.blue());
    this->ui_.spinBoxes[SpinBox::ALPHA]->setValue(this->color_.alpha());

    /*
     * Here, we are intentionally using HexRgb instead of HexArgb. Most online
     * sites (or other applications) will likely not include the alpha channel
     * in their output.
     */
    this->ui_.htmlEdit->setText(this->color_.name(QColor::HexRgb));
}

void ColorPickerDialog::ok()
{
    this->dialogConfirmed_ = true;
    this->close();
}

void ColorPickerDialog::initSpinBoxes(LayoutCreator<QWidget> &creator)
{
    auto spinBoxes = creator.setLayoutType<QGridLayout>();

    auto *red = this->ui_.spinBoxes[SpinBox::RED] = new QColSpinBox(this);
    auto *green = this->ui_.spinBoxes[SpinBox::GREEN] = new QColSpinBox(this);
    auto *blue = this->ui_.spinBoxes[SpinBox::BLUE] = new QColSpinBox(this);
    auto *alpha = this->ui_.spinBoxes[SpinBox::ALPHA] = new QColSpinBox(this);

    spinBoxes->addWidget(new QLabel("Red:"), 0, 0);
    spinBoxes->addWidget(red, 0, 1);

    spinBoxes->addWidget(new QLabel("Green:"), 1, 0);
    spinBoxes->addWidget(green, 1, 1);

    spinBoxes->addWidget(new QLabel("Blue:"), 2, 0);
    spinBoxes->addWidget(blue, 2, 1);

    spinBoxes->addWidget(new QLabel("Alpha:"), 3, 0);
    spinBoxes->addWidget(alpha, 3, 1);

    for (size_t i = 0; i < SpinBox::END; ++i)
    {
        QObject::connect(
            this->ui_.spinBoxes[i], QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int value) {
                this->selectColor(QColor(red->value(), green->value(),
                                         blue->value(), alpha->value()),
                                  false);
            });
    }
}

}  // namespace chatterino
