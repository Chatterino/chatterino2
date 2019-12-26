#include "widgets/dialogs/ColorPickerDialog.hpp"

// TODO(leon): Move ColorProvider to different directory?
#include "controllers/highlights/ColorProvider.hpp"

#include <QColorDialog>

namespace chatterino {

// TODO(leon): Replace magic values with constants

ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
    , color_()
    , dialogConfirmed_(false)
{
    // Set up UI element pointers
    for (int i = 0; i < 5; ++i)
    {
        this->ui_.recentColors.push_back(nullptr);
    }

    for (int i = 0; i < 5; ++i)
    {
        this->ui_.defaultColors.push_back(nullptr);
    }

    LayoutCreator<QWidget> layoutWidget(this->getLayoutContainer());
    auto layout = layoutWidget.setLayoutType<QVBoxLayout>().withoutMargin();

    LayoutCreator<QWidget> contentCreator(new QWidget());
    auto contents = contentCreator.setLayoutType<QHBoxLayout>();

    // Recently used colors
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        vbox.emplace<QLabel>("Recently used:");

        LayoutCreator<QWidget> rowCreator(new QWidget());
        auto row = rowCreator.setLayoutType<QHBoxLayout>();

        auto recentColors = ColorProvider::instance().recentColors();
        auto it = recentColors.begin();
        int i = 0;
        while (it != recentColors.end() && i < 5)
        {
            ColorButton *button = this->ui_.recentColors[i];
            row.emplace<ColorButton>(*it).assign(&button);

            QObject::connect(button, &QPushButton::clicked, [=] {
                this->selectColor(button->color(), false);
            });
            ++it;
            ++i;
        }

        vbox.append(rowCreator.getElement());
        contents.append(obj.getElement());
    }

    // Default colors
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();
        vbox.emplace<QLabel>("Default:");

        LayoutCreator<QWidget> rowCreator(new QWidget());
        auto row = rowCreator.setLayoutType<QHBoxLayout>();

        auto defaultColors = ColorProvider::instance().defaultColors();
        auto it = defaultColors.begin();
        int i = 0;
        while (it != defaultColors.end() && i < 5)
        {
            ColorButton *button = this->ui_.defaultColors[i];
            row.emplace<ColorButton>(*it).assign(&button);

            QObject::connect(button, &QPushButton::clicked, [=] {
                this->selectColor(button->color(), false);
            });
            ++it;
            ++i;
        }

        vbox.append(row.getElement());
        contents.append(obj.getElement());
    }

    // Currently selected color
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto hbox = obj.setLayoutType<QHBoxLayout>();
        hbox.emplace<QLabel>("Selected:");
        hbox.emplace<ColorButton>(initial).assign(&this->ui_.selectedColor);

        contents.append(obj.getElement());
    }

    // Color picker
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto vbox = obj.setLayoutType<QVBoxLayout>();

        // The actual color picker
        {
            LayoutCreator<QWidget> cpCreator(new QWidget());
            auto cpPanel = cpCreator.setLayoutType<QHBoxLayout>();
            cpPanel->setSizeConstraint(QLayout::SetFixedSize);

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

    this->adjustSize();
    this->selectColor(initial, false);
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
