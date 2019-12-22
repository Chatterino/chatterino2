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
    LayoutCreator<QWidget> layoutWidget(this->getLayoutContainer());
    auto layout = layoutWidget.setLayoutType<QVBoxLayout>().withoutMargin();

    for (int i = 0; i < 5; ++i)
    {
        this->ui_.recentColors.push_back(nullptr);
    }

    // Recently used colors
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto hbox = obj.setLayoutType<QHBoxLayout>();

        auto recentColors = ColorProvider::instance().recentColors();
        hbox.emplace<QLabel>("Recently used:");

        for (int i = 0; i < 5 && i < recentColors.size(); ++i)
        {
            ColorButton *button = this->ui_.recentColors[i];
            hbox.emplace<ColorButton>(recentColors[i]).assign(&button);

            QObject::connect(button, &QPushButton::clicked,
                             [=] { this->selectColor(button->color()); });
        }

        layout.append(obj.getElement());
    }

    // Default colors
    {
        // TODO(leon): Get default colors (from ColorProvider!?)
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
