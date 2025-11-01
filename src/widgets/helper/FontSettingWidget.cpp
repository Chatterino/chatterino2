#include "widgets/helper/FontSettingWidget.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "widgets/dialogs/font/FontSettingDialog.hpp"

#include <QHBoxLayout>
#include <QIcon>
#include <QToolButton>
#include <QVBoxLayout>

namespace chatterino {

void FontSettingWidget::updateCurrentLabel()
{
    QFont font = getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1);
    QString family = font.family();
    QString ptSize = QString::number(font.pointSize());
    this->currentLabel->setText(family + ", " + ptSize + "pt");
}

void FontSettingWidget::showDialog()
{
    static FontSettingDialog *instance = nullptr;

    if (!instance)
    {
        instance = new FontSettingDialog(getSettings()->chatFontFamily,
                                         getSettings()->chatFontSize,
                                         getSettings()->chatFontWeight, this);
        QObject::connect(instance, &QObject::destroyed, [&] {
            instance = nullptr;
        });
    }

    instance->show();
}

FontSettingWidget::FontSettingWidget(QWidget *parent)
    : QWidget(parent)
    , currentLabel(new QLabel)
    , listener([this] {
        this->updateCurrentLabel();
    })
{
    auto *layout = new QHBoxLayout;
    auto *button = new QToolButton;

    this->setLayout(layout);
    this->updateCurrentLabel();

    this->listener.add(getApp()->getFonts()->fontChanged);

    layout->addWidget(new QLabel("Font:"));
    layout->addStretch(1);
    layout->addWidget(this->currentLabel);
    layout->addWidget(button);
    layout->setContentsMargins(0, 0, 0, 0);

    button->setIcon(QIcon(":/buttons/edit.svg"));

    QObject::connect(button, &QToolButton::clicked, this,
                     &FontSettingWidget::showDialog);
}

}  // namespace chatterino
