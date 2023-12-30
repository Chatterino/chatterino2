#include "widgets/dialogs/ColorPickerDialog.hpp"

#include "common/Literals.hpp"
#include "providers/colors/ColorProvider.hpp"
#include "widgets/helper/color/ColorButton.hpp"
#include "widgets/helper/color/ColorDetails.hpp"
#include "widgets/helper/color/ColorSelect.hpp"

#include <QDialogButtonBox>

namespace {

using namespace chatterino;

constexpr size_t COLORS_PER_ROW = 5;

QGridLayout *makeColorGrid(const auto &items, auto *self)
{
    auto *layout = new QGridLayout;

    // TODO(nerix): use std::ranges::views::enumerate (C++ 23)
    for (std::size_t i = 0; auto color : items)
    {
        auto *button = new ColorButton(color);
        button->setMinimumWidth(40);
        QObject::connect(button, &ColorButton::clicked, self, [self, color]() {
            self->setColor(color);
        });

        layout->addWidget(button, static_cast<int>(i / COLORS_PER_ROW),
                          static_cast<int>(i % COLORS_PER_ROW));
        i++;
    }
    return layout;
}

}  // namespace

namespace chatterino {

using namespace literals;

ColorPickerDialog::ColorPickerDialog(QColor color, QWidget *parent)
    : BasePopup(
          {
              BaseWindow::EnableCustomFrame,
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
          },
          parent)
    , color_(color)
{
    this->setWindowTitle(u"Chatterino - Color picker"_s);
    this->setAttribute(Qt::WA_DeleteOnClose);

    auto *dialogContents = new QHBoxLayout;
    dialogContents->setContentsMargins(10, 10, 10, 10);
    {
        auto *buttons = new QVBoxLayout;
        buttons->addWidget(new QLabel(u"Recently used"_s));
        buttons->addLayout(
            makeColorGrid(ColorProvider::instance().recentColors(), this));

        buttons->addSpacing(10);

        buttons->addWidget(new QLabel(u"Default colors"_s));
        buttons->addLayout(
            makeColorGrid(ColorProvider::instance().defaultColors(), this));

        buttons->addStretch(1);
        auto *display = new ColorButton(this->color());
        QObject::connect(this, &ColorPickerDialog::colorChanged, display,
                         &ColorButton::setColor);
        buttons->addWidget(display);

        dialogContents->addLayout(buttons);
        dialogContents->addSpacing(10);
    }

    {
        auto *controls = new QVBoxLayout;

        auto *colorSelect = new ColorSelect(this->color());
        auto *colorDetails = new ColorDetails(this->color());

        QObject::connect(colorSelect, &ColorSelect::colorChanged, this,
                         &ColorPickerDialog::setColor);
        QObject::connect(colorDetails, &ColorDetails::colorChanged, this,
                         &ColorPickerDialog::setColor);
        QObject::connect(this, &ColorPickerDialog::colorChanged, colorSelect,
                         &ColorSelect::setColor);
        QObject::connect(this, &ColorPickerDialog::colorChanged, colorDetails,
                         &ColorDetails::setColor);

        controls->addWidget(colorSelect);
        controls->addWidget(colorDetails);

        dialogContents->addLayout(controls);
    }

    auto *dialogLayout = new QVBoxLayout(this->getLayoutContainer());
    dialogLayout->addLayout(dialogContents, 1);
    dialogLayout->addStretch(1);

    auto *buttonBox =
        new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QObject::connect(buttonBox, &QDialogButtonBox::accepted, this, [this] {
        emit this->colorConfirmed(this->color());
        this->close();
    });
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, this,
                     &ColorSelect::close);
    dialogLayout->addWidget(buttonBox, 0, Qt::AlignRight);
}

QColor ColorPickerDialog::color() const
{
    return this->color_;
}

void ColorPickerDialog::setColor(const QColor &color)
{
    if (color == this->color_)
    {
        return;
    }
    this->color_ = color;
    emit this->colorChanged(color);
}

}  // namespace chatterino
