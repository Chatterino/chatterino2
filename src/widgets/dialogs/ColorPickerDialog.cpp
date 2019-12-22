#include "widgets/dialogs/ColorPickerDialog.hpp"

#include "util/LayoutCreator.hpp"
// TODO(leon): Move ColorProvider to different directory?
#include "controllers/highlights/ColorProvider.hpp"

namespace chatterino {

// TODO(leon): Replace magic values with constants

ColorPickerDialog::ColorPickerDialog(const QColor &initial, QWidget *parent)
    : BaseWindow(BaseWindow::EnableCustomFrame, parent)
    , hasSelectedColor_(false)
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

            QObject::connect(button, &QPushButton::clicked, [=] {
                this->ui_.selectedColor->setColor(button->color());
            });
        }

        layout.append(obj.getElement());
    }

    // Default colors
    {
        // TODO(leon): Get default colors (from ColorProvider!?)
    }

    // Color selection
    {
        // TODO(leon)
    }

    // Currently selected color, may want to consolidate this with a different area
    {
        LayoutCreator<QWidget> obj(new QWidget());
        auto hbox = obj.setLayoutType<QHBoxLayout>();
        hbox.emplace<QLabel>("Selected:");
        hbox.emplace<ColorButton>(initial).assign(&this->ui_.selectedColor);
        layout.append(obj.getElement());
    }

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

void ColorPickerDialog::closeEvent(QCloseEvent *)
{
    this->closed.invoke();
}

void ColorPickerDialog::ok()
{
    this->hasSelectedColor_ = true;
    this->close();
}

QColor ColorPickerDialog::selectedColor() const
{
    if (this->hasSelectedColor_)
        return this->ui_.selectedColor->color();

    // If the Cancel button was clicked, return the invalid color
    return QColor();
}

}  // namespace chatterino
