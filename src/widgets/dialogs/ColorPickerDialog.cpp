#include "widgets/dialogs/ColorPickerDialog.hpp"

#include "util/LayoutCreator.hpp"
// TODO(leon): Move ColorProvider to different directory?
#include "controllers/highlights/ColorProvider.hpp"

#include <QColorDialog>

namespace chatterino {

// TODO(leon): Replace magic values with constants

ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
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

    // Recently used colors
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto hbox = obj.setLayoutType<QHBoxLayout>();

        hbox.emplace<QLabel>("Recently used:");

        auto recentColors = ColorProvider::instance().recentColors();
        auto it = recentColors.begin();
        int i = 0;
        while (it != recentColors.end() && i < 5)
        {
            ColorButton *button = this->ui_.recentColors[i];
            hbox.emplace<ColorButton>(*it).assign(&button);

            QObject::connect(button, &QPushButton::clicked,
                             [=] { this->selectColor(button->color()); });
            ++it;
            ++i;
        }

        layout.append(obj.getElement());
    }

    // Default colors
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto hbox = obj.setLayoutType<QHBoxLayout>();
        hbox.emplace<QLabel>("Default:");

        auto defaultColors = ColorProvider::instance().defaultColors();
        auto it = defaultColors.begin();
        int i = 0;
        while (it != defaultColors.end() && i < 5)
        {
            ColorButton *button = this->ui_.defaultColors[i];
            hbox.emplace<ColorButton>(*it).assign(&button);

            QObject::connect(button, &QPushButton::clicked,
                             [=] { this->selectColor(button->color()); });
            ++it;
            ++i;
        }

        layout.append(obj.getElement());
    }

    // Currently selected color
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto hbox = obj.setLayoutType<QHBoxLayout>();
        hbox.emplace<QLabel>("Selected:");
        hbox.emplace<ColorButton>(initial).assign(&this->ui_.selectedColor);

        QObject::connect(this->ui_.selectedColor, &QPushButton::clicked, [=] {
            auto customColor = QColorDialog::getColor(
                this->ui_.selectedColor->color(), this, tr("Select Color"),
                QColorDialog::ShowAlphaChannel);

            if (customColor.isValid())
                this->selectColor(customColor);
        });

        layout.append(obj.getElement());
    }

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
}

QColor ColorPickerDialog::selectedColor() const
{
    if (this->dialogConfirmed_)
        return this->ui_.selectedColor->color();

    // If the Cancel button was clicked, return the invalid color
    return QColor();
}

void ColorPickerDialog::closeEvent(QCloseEvent *)
{
    this->closed.invoke();
}

void ColorPickerDialog::selectColor(const QColor &color)
{
    this->ui_.selectedColor->setColor(color);
}

void ColorPickerDialog::ok()
{
    this->dialogConfirmed_ = true;
    this->close();
}

}  // namespace chatterino
