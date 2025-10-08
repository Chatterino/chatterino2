#include "widgets/helper/FontSettingWidget.hpp"
#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"

#include <QFontDialog>

namespace chatterino {

void FontSettingWidget::updateCurrentLabel()
{
    QFont font = getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1);
    auto family = font.family();
    auto ptSize = QString::number(font.pointSize());
    this->currentLabel->setText(family + ", " + ptSize + "pt");
}

FontSettingWidget::FontSettingWidget(QWidget *parent)
    : QWidget(parent)
    , currentLabel(new QLabel)
{
    auto *button = new QPushButton;

    button->setIcon(QIcon(":/buttons/edit.svg"));

    QObject::connect(button, &QPushButton::clicked, this, [this]() {
        bool ok = false;
        QFont prev = getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1);
        QFont font = QFontDialog::getFont(&ok, prev, this);

        if (ok)
        {
            getSettings()->chatFontFamily = font.family();
            getSettings()->chatFontSize = font.pointSize();
            getSettings()->chatFontWeight = font.weight();
        }
    });

    auto *layout = new QHBoxLayout;

    layout->addWidget(new QLabel("Font:"));
    layout->addStretch(1);
    layout->addWidget(this->currentLabel);
    layout->addWidget(button);
    layout->setContentsMargins(0, 0, 0, 0);

    this->listener.addSetting(getSettings()->chatFontFamily);
    this->listener.addSetting(getSettings()->chatFontSize);
    this->listener.addSetting(getSettings()->chatFontWeight);
    this->listener.setCB([this] {
        this->updateCurrentLabel();
    });

    this->updateCurrentLabel();
    this->setLayout(layout);
}

}  // namespace chatterino
